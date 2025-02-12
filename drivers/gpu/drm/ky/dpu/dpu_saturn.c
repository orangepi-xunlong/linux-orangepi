// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/mfd/syscon.h>
#include <linux/pm_qos.h>
#include <linux/regmap.h>
#include <drm/drm_atomic.h>
#include <drm/drm_blend.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_gem.h>
#include <drm/drm_writeback.h>
#include "dpu_saturn.h"
#include "saturn_fbcmem.h"
#include "../ky_cmdlist.h"
#include "../ky_dmmu.h"
#include "../ky_dpu_reg.h"
#include "../ky_drm.h"
#include "../ky_wb.h"
#include <video/display_timing.h>
#include <dt-bindings/display/ky-dpu.h>

#define CREATE_TRACE_POINTS
#include "dpu_trace.h"
#include "dpu_debug.h"

#define TOTAL_RDMA_MEMSIZE	(68 * 1024) /* 68KB */
#define YUV2RGB_COEFFS 12

//RDMA_FMT_YUV_420_P1_8, RDMA_FMT_YUV_420_P1_10 not support by hardware, has checked with asic
//rdma hardware support RDMA_FMT_BGRA_16161616/RDMA_FMT_RGBA_16161616 formats are not  support by fourcc
static const struct dpu_format_id primary_fmts[] = {
	{ DRM_FORMAT_ABGR2101010, 1, 32 }, //RDMA_FMT_ABGR_2101010
	{ DRM_FORMAT_ARGB8888, 4, 32 }, //RDMA_FMT_ARGB_8888
	{ DRM_FORMAT_ABGR8888, 5, 32 }, //RDMA_FMT_ABGR_8888
	{ DRM_FORMAT_RGBA8888, 6, 32 }, //RDMA_FMT_RGBA_8888
	{ DRM_FORMAT_BGRA8888, 7, 32 }, //RDMA_FMT_BGRA_8888
	{ DRM_FORMAT_XRGB8888, 8, 32 }, //RDMA_FMT_XRGB_8888
	{ DRM_FORMAT_XBGR8888, 9, 32 }, //RDMA_FMT_XBGR_8888
	{ DRM_FORMAT_RGBX8888, 10, 32 }, //RDMA_FMT_RGBX_8888
	{ DRM_FORMAT_BGRX8888, 11, 32 }, //RDMA_FMT_BGRX_8888
	{ DRM_FORMAT_RGB565, 22, 16 }, //RDMA_FMT_RGB_565
	{ DRM_FORMAT_BGR565, 23, 16 }, //RDMA_FMT_BGR_565
	/*
	{ DRM_FORMAT_ARGB2101010,	 0 }, //RDMA_FMT_ARGB_2101010
	{ DRM_FORMAT_RGBA1010102,	 2 }, //RDMA_FMT_RGBA_2101010
	{ DRM_FORMAT_BGRA1010102,	 3 }, //RDMA_FMT_BGRA_2101010
	{ DRM_FORMAT_RGB888,		12 }, //RDMA_FMT_RGB_888
	{ DRM_FORMAT_BGR888,        13 }, //RDMA_FMT_BGR_888
	{ DRM_FORMAT_RGBA5551,      14 }, //RDMA_FMT_RGBA_5551
	{ DRM_FORMAT_BGRA5551,      15 }, //RDMA_FMT_BGRA_5551
	{ DRM_FORMAT_ABGR1555,      16 }, //RDMA_FMT_ABGR_1555
	{ DRM_FORMAT_ARGB1555,      17 }, //RDMA_FMT_ARGB_1555
	{ DRM_FORMAT_RGBX5551,      18 }, //RDMA_FMT_RGBX_5551
	{ DRM_FORMAT_BGRX5551,      19 }, //RDMA_FMT_BGRX_5551
	{ DRM_FORMAT_XBGR1555,      20 }, //RDMA_FMT_XBGR_1555
	{ DRM_FORMAT_XRGB1555,      21 }, //RDMA_FMT_XRGB_1555
	{ DRM_FORMAT_ARGB16161616F, 24 }, //RDMA_FMT_ARGB_16161616
	{ DRM_FORMAT_ABGR16161616F, 25 }, //RDMA_FMT_ABGR_16161616
	{ DRM_FORMAT_XYUV8888,      32 }, //RDMA_FMT_XYUV_444_P1_8, uv_swap has no corresponding fourcc format
	{ DRM_FORMAT_Y410,          33 }, //RDMA_FMT_XYUV_444_P1_10, uv_swap has no corresponding fourcc format
	{ DRM_FORMAT_YUYV,          34 }, //RDMA_FMT_VYUY_422_P1_8
	{ DRM_FORMAT_YVYU,          34 }, //RDMA_FMT_VYUY_422_P1_8, uv_swap = 1
	{ DRM_FORMAT_UYVY,          35 }, //RDMA_FMT_YVYU_422_P1_8
	{ DRM_FORMAT_VYUY,          35 }, //RDMA_FMT_YVYU_422_P1_8, uv_swap = 1
	*/
	{ DRM_FORMAT_YUV420_8BIT,   37, 12 }, //DRM_FORMAT_YUV420_8BIT for AFBC
	{ DRM_FORMAT_NV12,          37, 12 }, //RDMA_FMT_YUV_420_P2_8
	/*
	{ DRM_FORMAT_NV21,          37 }, //RDMA_FMT_YUV_420_P2_8, uv_swap = 1
	{ DRM_FORMAT_YUV420,        38 }, //RDMA_FMT_YUV_420_P3_8
	{ DRM_FORMAT_YVU420,        38 }, //RDMA_FMT_YUV_420_P3_8, uv_swap = 1
	{ DRM_FORMAT_YUV420_10BIT,  39 }, //RDMA_FMT_YUV_420_P1_10 //DPU not support, DO NOT use
	*/
	{ DRM_FORMAT_P010,          40, 24 }, //RDMA_FMT_YUV_420_P2_10
	/*
	{ DRM_FORMAT_Q410,          41 }, //DPU not support
	{ DRM_FORMAT_Q401,          41 }, //DPU not support
	*/
};

const struct ky_hw_rdma saturn_rdmas[] = {
	{FORMAT_RGB | FORMAT_RAW_YUV | FORMAT_AFBC, ROTATE_COMMON | ROTATE_AFBC_90_270},
	{FORMAT_RGB | FORMAT_RAW_YUV | FORMAT_AFBC, ROTATE_COMMON | ROTATE_AFBC_90_270},
	{FORMAT_RGB | FORMAT_RAW_YUV | FORMAT_AFBC, ROTATE_COMMON | ROTATE_AFBC_90_270},
	{FORMAT_RGB | FORMAT_RAW_YUV | FORMAT_AFBC, ROTATE_COMMON | ROTATE_AFBC_90_270},
};

const u32 saturn_fbcmem_sizes[] = {
	68 * 1024,
	68 * 1024,
};
EXPORT_SYMBOL(saturn_fbcmem_sizes);

const u32 saturn_rdma_fixed_fbcmem_sizes[] = {
	34 * 1024,
	34 * 1024,
	34 * 1024,
	34 * 1024,
};

static const s16
ky_yuv2rgb_coefs[DRM_COLOR_ENCODING_MAX][DRM_COLOR_RANGE_MAX][YUV2RGB_COEFFS] = {
	[DRM_COLOR_YCBCR_BT601][DRM_COLOR_YCBCR_LIMITED_RANGE] = {
		1192,    0, 1635, -223,
		1192, -403, -833, 136,
		1192, 2065,    0, -277,
	},
	[DRM_COLOR_YCBCR_BT601][DRM_COLOR_YCBCR_FULL_RANGE] = {
		1024,    0, 1436, -179,
		1024, -354, -732,  136,
		1024, 1814,    0, -227,
	},
	[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE] = {
		1192,    0, 1836, -248,
		1192, -218, -546,   77,
		1192, 2163,    0, -289,
	},
	[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_FULL_RANGE] = {
		1024,	 0, 1613, -202,
		1024, -192, -479,   84,
		1024, 1900,    0, -238,
	},
	[DRM_COLOR_YCBCR_BT2020][DRM_COLOR_YCBCR_LIMITED_RANGE] = {
		1196,	 0, 1724,  -937,
		1196, -192, -668,   355,
		1196, 2200,    0, -1175,
	},
	[DRM_COLOR_YCBCR_BT2020][DRM_COLOR_YCBCR_FULL_RANGE] = {
		1024,    0, 1510, -755,
		1024, -168, -585,  377,
		1024, 1927,    0, -963,
	}
};

const struct ky_hw_rdma saturn_le_rdmas[] = {
	/* TODO: set max_yuv_height to 1088 instead of 1080 due to vpu fw issue */
	{FORMAT_RGB | FORMAT_AFBC, ROTATE_COMMON},
	{FORMAT_RGB | FORMAT_RAW_YUV | FORMAT_AFBC, ROTATE_COMMON | ROTATE_AFBC_90_270},
	{FORMAT_RGB, ROTATE_COMMON},
	{FORMAT_RGB | FORMAT_RAW_YUV | FORMAT_AFBC, ROTATE_COMMON | ROTATE_AFBC_90_270},
};

const u32 saturn_le_fbcmem_sizes[] = {
	44544,	//43.5k
	35712,	//34.875k
};
EXPORT_SYMBOL(saturn_le_fbcmem_sizes);

const u32 saturn_le_rdma_fixed_fbcmem_sizes[] = {
	11776,		//43.5k
	32 * 1024,
	2944,		//2.875k
	32 * 1024,
};

static atomic_t mclk_cnt = ATOMIC_INIT(0);;
bool dpu_mclk_exclusive_get(void)
{
	if (0 == atomic_cmpxchg(&mclk_cnt, 0, 1))
		return true;
	else
		return false;
}
EXPORT_SYMBOL(dpu_mclk_exclusive_get);

void dpu_mclk_exclusive_put(void)
{
	atomic_set(&mclk_cnt, 0);
}
EXPORT_SYMBOL(dpu_mclk_exclusive_put);

struct ky_hw_device ky_dp_devices[DP_MAX_DEVICES] = {
	[SATURN_HDMI] = {
		.base = NULL,		/* Parsed by dts */
		.phy_addr = 0x0,	/* Parsed by dts */
		.plane_nums = 8,
		.rdma_nums = ARRAY_SIZE(saturn_le_rdmas),
		.rdmas = saturn_le_rdmas,
		.n_formats = ARRAY_SIZE(primary_fmts),
		.formats = primary_fmts,
		.n_fbcmems = ARRAY_SIZE(saturn_le_fbcmem_sizes),
		.fbcmem_sizes = saturn_le_fbcmem_sizes,
		.rdma_fixed_fbcmem_sizes = saturn_le_rdma_fixed_fbcmem_sizes,
		.solid_color_shift = 0,
		.hdr_coef_size = 135,
		.scale_coef_size = 48,
		.is_hdmi = true,
	},
	[SATURN_LE] = {
		.base = NULL,		/* Parsed by dts */
		.phy_addr = 0x0,	/* Parsed by dts */
		.plane_nums = 8,
		.rdma_nums = ARRAY_SIZE(saturn_le_rdmas),
		.rdmas = saturn_le_rdmas,
		.n_formats = ARRAY_SIZE(primary_fmts),
		.formats = primary_fmts,
		.n_fbcmems = ARRAY_SIZE(saturn_le_fbcmem_sizes),
		.fbcmem_sizes = saturn_le_fbcmem_sizes,
		.rdma_fixed_fbcmem_sizes = saturn_le_rdma_fixed_fbcmem_sizes,
		.solid_color_shift = 0,
		.hdr_coef_size = 135,
		.scale_coef_size = 48,
		.is_hdmi = false,
	},
};
EXPORT_SYMBOL(ky_dp_devices);

static int dpu_parse_dt(struct ky_dpu *dpu, struct device_node *np)
{
	struct dpu_clk_context *clk_ctx = &dpu->clk_ctx;

	clk_ctx->pxclk = of_clk_get_by_name(np, "pxclk");
	if (IS_ERR(clk_ctx->pxclk)) {
		pr_debug("%s, read pxclk failed from dts!\n", __func__);
	}

	clk_ctx->mclk = of_clk_get_by_name(np, "mclk");
	if (IS_ERR(clk_ctx->mclk)) {
		pr_debug("%s, read mclk failed from dts!\n", __func__);
	}

	clk_ctx->hclk = of_clk_get_by_name(np, "hclk");
	if (IS_ERR(clk_ctx->hclk)) {
		pr_debug("%s, read hclk failed from dts!\n", __func__);
	}

	clk_ctx->escclk = of_clk_get_by_name(np, "escclk");
	if (IS_ERR(clk_ctx->escclk)) {
		pr_debug("%s, read escclk failed from dts!\n", __func__);
	}

	clk_ctx->bitclk = of_clk_get_by_name(np, "bitclk");
	if (IS_ERR(clk_ctx->bitclk)) {
		pr_debug("%s, read bitclk failed from dts!\n", __func__);
	}

	clk_ctx->hmclk = of_clk_get_by_name(np, "hmclk");
	if (IS_ERR(clk_ctx->hmclk)) {
		pr_debug("%s, read hmclk failed from dts!\n", __func__);
	}

	if (of_property_read_u32(np, "ky-dpu-min-mclk", &dpu->min_mclk))
		dpu->min_mclk = DPU_MCLK_DEFAULT;

	if (of_property_read_bool(np, "ky-dpu-auto-fc"))
		dpu->enable_auto_fc = 0;

	if (of_property_read_u32(np, "ky-dpu-bitclk", &dpu->bitclk))
		dpu->bitclk = DPU_BITCLK_DEFAULT;

	if (of_property_read_u32(np, "ky-dpu-escclk", &dpu->escclk))
		dpu->escclk = DPU_ESCCLK_DEFAULT;

	return 0;
}


static unsigned int dpu_get_bpp(u32 format)
{
	unsigned int i = 0;

	for(i = 0; i < ARRAY_SIZE(primary_fmts); i++) {
		if (format == primary_fmts[i].format)
			return primary_fmts[i].bpp;
	}

	DRM_ERROR("format 0x%x is not supported!\n", format);
	return KY_DPU_INVALID_FORMAT_ID;
}

int dpu_calc_plane_mclk_bw(struct drm_plane *plane, \
		struct drm_plane_state *new_state)
{
	/* For some platform without aclk, mclk = max(aclk, mclk) */
	uint64_t calc_mclk = 0;
	uint64_t Fpixclk_hblk = 0;
	struct drm_plane_state *state = NULL;
	struct drm_crtc *crtc = new_state->crtc;
	struct drm_display_mode *mode = NULL;
	struct ky_plane_state *ky_plane_state = NULL;
	uint64_t tmp;
	uint64_t hact = 0;
	uint64_t fps = 0;
	bool scl_en = 0;

	struct drm_framebuffer *fb = new_state->fb;
	const struct drm_format_info *format = fb->format;
	unsigned int bpp = dpu_get_bpp(format->format);
	uint64_t calc_bandwidth = 0;

	struct ky_dpu *dpu = NULL;

	state = new_state;
	if (!crtc)
		return 0;

	dpu = crtc_to_dpu(crtc);
	if (!dpu->enable_auto_fc)
		return 0;

	/* prepare calc mclk and bw */
	mode = &crtc->mode;
	hact = mode->hdisplay;
	fps = mode->clock * 1000 / (mode->htotal * (u16)mode->vtotal);
	ky_plane_state = to_ky_plane_state(state);
	scl_en = ky_plane_state->use_scl;
	Fpixclk_hblk = hact * (mode->vtotal) * fps; /* MHZ */

	trace_dpu_fpixclk_hblk(hact, fps, (uint64_t)mode->vtotal, Fpixclk_hblk);

	/* calculate mclk and bandwidth */
	if (scl_en) {
		unsigned int C = 0;
		uint64_t width_ratio = 0;	/* (scl_in_width/hact) * 1000000 */
		uint64_t height_ratio = 0;	/* roundup(scl_in_height/scl_out_height) */
		uint64_t scl_in_width, scl_in_height, scl_out_width, scl_out_height;

		if (state->rotation == DRM_MODE_ROTATE_90 || state->rotation == DRM_MODE_ROTATE_270) {
			scl_in_width  = state->src_h >> 16;
			scl_in_height = state->src_w >> 16;
		} else {
			scl_in_width  = state->src_w >> 16;
			scl_in_height = state->src_h >> 16;
		}

		scl_out_width = state->crtc_w;
		scl_out_height = state->crtc_h;

		C = min(2 * scl_in_width / scl_out_width, 4ULL);	/* sclaer after rdma */
		ky_plane_state->afbc_effc = 4 / C;

		//width_ratio = scl_in_width * MHZ2HZ / hact;
		tmp = scl_in_width * MHZ2HZ;
		do_div(tmp, hact);
		width_ratio = tmp;
		height_ratio = DIV_ROUND_UP_ULL(scl_in_height, scl_out_height) * MHZ2HZ;
		//calc_mclk = Fpixclk_hblk * max(width_ratio, MHZ2HZ) / MHZ2HZ * max(height_ratio, MHZ2HZ) / MHZ2HZ / C;
		tmp = Fpixclk_hblk * max(width_ratio, MHZ2HZ);
		do_div(tmp, MHZ2HZ);
		tmp = tmp * max(height_ratio, MHZ2HZ);
		do_div(tmp, MHZ2HZ);
		do_div(tmp, C);
		calc_mclk = tmp;

		if (calc_mclk > DPU_MCLK_MAX) {
			DRM_INFO("plane:%d mclk too large %lld\n", state->zpos, calc_mclk);
			DRM_INFO("hact = %lld, fps = %lld, vtotal = %d, Fpixclk_hblk = %lld\n", hact, fps, (u16)mode->vtotal, Fpixclk_hblk);
			DRM_INFO("scl_in_width = %lld, scl_in_height = %lld, scl_out_width = %lld, scl_out_height = %lld, bpp = %d, C = %d\n", scl_in_width, scl_in_height, scl_out_width, scl_out_height, bpp, C);
			return -EINVAL;
		}

		//calc_bandwidth = Fpixclk_hblk * bpp * width_ratio / MHZ2HZ * height_ratio / MHZ2HZ / 8;
		tmp = Fpixclk_hblk * bpp * width_ratio;
		do_div(tmp, MHZ2HZ);
		tmp = tmp * height_ratio;
		do_div(tmp, MHZ2HZ);
		tmp = tmp >> 3;
		calc_bandwidth = tmp;
		if (calc_bandwidth > (DPU_MAX_QOS_REQ * MHZ2KHZ)) {
			DRM_INFO("plane:%d bandwidth too large %lld\n", state->zpos, calc_bandwidth);
			DRM_INFO("hact = %lld, fps = %lld, vtotal = %d, Fpixclk_hblk = %lld\n", hact, fps, (u16)mode->vtotal, Fpixclk_hblk);
			DRM_INFO("scl_in_width = %lld, scl_in_height = %lld, scl_out_width = %lld, scl_out_height = %lld, bpp = %d, C = %d\n", scl_in_width, scl_in_height, scl_out_width, scl_out_height, bpp, C);
			return -EINVAL;
		}
		trace_dpu_mclk_scl(scl_in_width, scl_in_height, scl_out_width, scl_out_height, bpp, C, width_ratio, height_ratio);
	} else {
		unsigned long img_width = 0;

		if (state->rotation == DRM_MODE_ROTATE_90 || state->rotation == DRM_MODE_ROTATE_270)
			img_width = state->src_h >> 16;
		else
			img_width = state->src_w >> 16;

		trace_u64_data("no scl img_width", img_width);
		//calc_mclk = ( hact + 32 ) * MHZ2HZ / hact * Fpixclk_hblk / 2 / MHZ2HZ;
		tmp = ( hact + 32 ) * MHZ2HZ;
		do_div(tmp, hact);
		tmp = tmp * Fpixclk_hblk;
		tmp = tmp >> 1;
		do_div(tmp, MHZ2HZ);
		calc_mclk = tmp;
		if (calc_mclk > DPU_MCLK_MAX) {
			DRM_INFO("plane:%d mclk too large %lld\n", state->zpos, calc_mclk);
			DRM_INFO("img_width = %ld\n", img_width);
			return -EINVAL;
		}
		//calc_bandwidth = Fpixclk_hblk * bpp * img_width / hact / 8;
		tmp = Fpixclk_hblk * bpp * img_width;
		do_div(tmp, hact);
		do_div(tmp, 8);
		calc_bandwidth = tmp;
		if (calc_bandwidth > (DPU_MAX_QOS_REQ * MHZ2KHZ)) {
			DRM_INFO("plane:%d bandwidth too large %lld\n", state->zpos, calc_bandwidth);
			DRM_INFO("img_width = %ld\n", img_width);
			return -EINVAL;
		}
	}

	dpu = crtc_to_dpu(crtc);
	if (calc_mclk < dpu->min_mclk)
		calc_mclk = dpu->min_mclk;

	ky_plane_state->mclk = calc_mclk;

	/* add some buffer for MMU and AFBC Header */
	//calc_bandwidth = calc_bandwidth * 108 / 100;
	tmp = calc_bandwidth * 108;
	do_div(tmp, 100);
	calc_bandwidth = tmp;
	ky_plane_state->bw = calc_bandwidth;

	trace_u64_data("plane calc_mclk", calc_mclk);
	trace_u64_data("plane calc_bw", calc_bandwidth);
	return 0;
}

static int dpu_update_clocks(struct ky_dpu *dpu, uint64_t mclk)
{
	struct dpu_clk_context *clk_ctx = &dpu->clk_ctx;
	uint64_t cur_mclk = 0;
	int ret = 0;
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;

	if (!hwdev->is_hdmi) {
		trace_u64_data("update mclk", mclk);
		cur_mclk = clk_get_rate(clk_ctx->mclk);
		if (cur_mclk == mclk)
			return 0;
		if (dpu_mclk_exclusive_get()) {
			ret = clk_set_rate(clk_ctx->mclk, mclk);
			if (ret) {
				trace_u64_data("Failed to set mclk", mclk);
				DRM_ERROR("Failed to set DPU MCLK %lld %d\n", mclk, ret);
			} else {
				dpu_mclk_exclusive_put();
				dpu->cur_mclk = clk_get_rate(clk_ctx->mclk);
				trace_u64_data("Pass to set mclk", mclk);
			}
		} else {
			if (mclk > dpu->cur_mclk) {
				trace_u64_data("MCLK using by other module", mclk);
				DRM_ERROR("Mclk using by other module %lld\n", mclk);
			} else
				dpu->cur_mclk = mclk;

			return 0;
		}
	}

	return 0;
}

static int dpu_update_bw(struct ky_dpu *dpu, uint64_t bw)
{
	uint64_t __maybe_unused tmp;

	trace_u64_data("update bw", bw);

	if (dpu->cur_bw == bw)
		return 0;

	return 0;
}

static int dpu_finish_uboot(struct ky_dpu *dpu)
{
	void __iomem *base;
	void __iomem *hdmi;
	u32 value;

	DRM_DEBUG("%s() type %d\n", __func__, dpu->type);

	if (dpu->type == HDMI) {
		base = (void __iomem *)ioremap(0xC0440000, 0x2A000);
		hdmi = (void __iomem *)ioremap(0xC0400500, 0x200);

		// hdmi dpu ctl regs
		writel(0x00, base + 0x560);
		writel(0x01, base + 0x56c);
		// writel(0x00, base + 0x58c);

		// hdmi dpu int regs
		writel(0x00, base + 0x910);
		writel(0x00, base + 0x938);
		//writel(0x00, base + 0x960);

		// hdmi close pll clock
		writel(0x00, hdmi + 0xe4);

		value = readl_relaxed(base + 0x910);
		DRM_DEBUG("%s hdmi int reg4 0x910:0x%x\n", __func__, value);
		value = readl_relaxed(base + 0x938);
		DRM_DEBUG("%s hdmi int reg14 0x938:0x%x\n", __func__, value);
		value = readl_relaxed(base + 0x960);
		DRM_DEBUG("%s hdmi int reg24 0x960:0x%x\n", __func__, value);
		value = readl_relaxed(base + 0x560);
		DRM_DEBUG("%s hdmi ctl reg24 0x560:0x%x\n", __func__, value);
		value = readl_relaxed(base + 0x56c);
		DRM_DEBUG("%s hdmi ctl reg27 0x56c:0x%x\n", __func__, value);
		value = readl_relaxed(base + 0x58c);
		DRM_DEBUG("%s hdmi ctl reg35 0x58c:0x%x\n", __func__, value);

		udelay(100);
		iounmap(base);
		iounmap(hdmi);
	} else if (dpu->type == DSI) {
		base = (void __iomem *)ioremap(0xc0340000, 0x2A000);

		// mipi dsi dpu ctl regs
		writel(0x00, base + 0x560);
		writel(0x01, base + 0x56c);
		// writel(0x00, base + 0x58c);

		// mipi dsi dpu int regs
		writel(0x00, base + 0x910);
		writel(0x00, base + 0x938);
		//writel(0x00, base + 0x960);

		value = readl_relaxed(base + 0x910);
		DRM_DEBUG("%s mipi dsi int reg4 0x910:0x%x\n", __func__, value);
		value = readl_relaxed(base + 0x938);
		DRM_DEBUG("%s mipi dsi int reg14 0x938:0x%x\n", __func__, value);
		value = readl_relaxed(base + 0x960);
		DRM_DEBUG("%s mipi dsi int reg24 0x960:0x%x\n", __func__, value);
		value = readl_relaxed(base + 0x560);
		DRM_DEBUG("%s mipi dsi ctl reg24 0x560:0x%x\n", __func__, value);
		value = readl_relaxed(base + 0x56c);
		DRM_DEBUG("%s mipi dsi ctl reg27 0x56c:0x%x\n", __func__, value);
		value = readl_relaxed(base + 0x58c);
		DRM_DEBUG("%s mipi dsi ctl reg35 0x58c:0x%x\n", __func__, value);

		udelay(100);
		iounmap(base);
	} else {
		return 0;
	}

	return 0;
}

static int dpu_enable_clocks(struct ky_dpu *dpu)
{
	struct dpu_clk_context *clk_ctx = &dpu->clk_ctx;
	struct drm_crtc *crtc = &dpu->crtc;
	struct drm_display_mode *mode = &crtc->mode;
	uint64_t clk_val;
	uint64_t set_clk_val;
	struct ky_drm_private *priv;
	struct ky_hw_device *hwdev;

	DRM_DEBUG("%s() type %d\n", __func__, dpu->type);

	if (dpu->logo_booton) {
		if (dpu->type == HDMI) {
			clk_prepare_enable(clk_ctx->hmclk);
		} else if (dpu->type == DSI) {
			clk_prepare_enable(clk_ctx->pxclk);
			clk_prepare_enable(clk_ctx->mclk);
			clk_prepare_enable(clk_ctx->hclk);
			clk_prepare_enable(clk_ctx->escclk);
			clk_prepare_enable(clk_ctx->bitclk);
		}
		udelay(10);

		return 0;
	}

	priv = dpu->crtc.dev->dev_private;
	hwdev = priv->hwdev;

	if (hwdev->is_hdmi) {
		clk_prepare_enable(clk_ctx->hmclk);

		clk_val = clk_get_rate(clk_ctx->hmclk);
		if(clk_val != DPU_MCLK_DEFAULT){
			clk_val = clk_round_rate(clk_ctx->hmclk, DPU_MCLK_DEFAULT);
			if (dpu_mclk_exclusive_get()) {
				clk_set_rate(clk_ctx->hmclk, clk_val);
				DRM_DEBUG("set hdmi mclk=%lld\n", clk_val);
				dpu_mclk_exclusive_put();
			}
		}

		clk_val = clk_get_rate(clk_ctx->hmclk);
		DRM_DEBUG("get hdmi mclk=%lld\n", clk_val);

		udelay(10);
	} else {
		clk_prepare_enable(clk_ctx->pxclk);
		clk_prepare_enable(clk_ctx->mclk);
		clk_prepare_enable(clk_ctx->hclk);
		clk_prepare_enable(clk_ctx->escclk);
		clk_prepare_enable(clk_ctx->bitclk);

		set_clk_val = mode->clock * 1000;
		DRM_INFO("pxclk set_clk_val %lld\n", set_clk_val);

		if (set_clk_val) {
			set_clk_val = clk_round_rate(clk_ctx->pxclk, set_clk_val);
			clk_val = clk_get_rate(clk_ctx->pxclk);
			if(clk_val != set_clk_val){
				clk_set_rate(clk_ctx->pxclk, set_clk_val);
				DRM_DEBUG("set pxclk=%lld\n", set_clk_val);
			}
		}

		clk_val = clk_get_rate(clk_ctx->mclk);
		if(clk_val != DPU_MCLK_DEFAULT){
			clk_val = clk_round_rate(clk_ctx->mclk, DPU_MCLK_DEFAULT);
			if (dpu_mclk_exclusive_get()) {
				clk_set_rate(clk_ctx->mclk, clk_val);
				DRM_DEBUG("set mclk=%lld\n", clk_val);
				dpu_mclk_exclusive_put();
			}
		}

		clk_val = clk_get_rate(clk_ctx->escclk);
		set_clk_val = dpu->escclk;
		if(clk_val != set_clk_val){
			clk_val = clk_round_rate(clk_ctx->escclk, set_clk_val);
			clk_set_rate(clk_ctx->escclk, clk_val);
			DRM_DEBUG("set escclk=%lld\n", clk_val);
		}

		clk_val = clk_get_rate(clk_ctx->bitclk);
		set_clk_val = dpu->bitclk;
		if(clk_val != set_clk_val){
			clk_val = clk_round_rate(clk_ctx->bitclk, set_clk_val);
			clk_set_rate(clk_ctx->bitclk, clk_val);
			DRM_DEBUG("set bitclk=%lld\n", clk_val);
		}

		clk_val = clk_get_rate(clk_ctx->pxclk);
		DRM_DEBUG("get pxclk=%lld\n", clk_val);
		clk_val = clk_get_rate(clk_ctx->mclk);
		DRM_DEBUG("get mclk=%lld\n", clk_val);
		clk_val = clk_get_rate(clk_ctx->hclk);
		DRM_DEBUG("get hclk=%lld\n", clk_val);
		clk_val = clk_get_rate(clk_ctx->escclk);
		DRM_DEBUG("get escclk=%lld\n", clk_val);
		clk_val = clk_get_rate(clk_ctx->bitclk);
		DRM_DEBUG("get bitclk=%lld\n", clk_val);

		udelay(10);
	}

	trace_dpu_enable_clocks(dpu->dev_id);

	return 0;
}

static int dpu_disable_clocks(struct ky_dpu *dpu)
{
	struct dpu_clk_context *clk_ctx = &dpu->clk_ctx;
	struct ky_drm_private *priv;
	struct ky_hw_device *hwdev;

	DRM_DEBUG("%s() type %d\n", __func__, dpu->type);

	trace_dpu_disable_clocks(dpu->dev_id);

	if (dpu->logo_booton) {
		dpu_finish_uboot(dpu);
		if (dpu->type == HDMI) {
			clk_disable_unprepare(clk_ctx->hmclk);
		} else if (dpu->type == DSI) {
			clk_disable_unprepare(clk_ctx->pxclk);
			clk_disable_unprepare(clk_ctx->mclk);
			clk_disable_unprepare(clk_ctx->hclk);
			clk_disable_unprepare(clk_ctx->escclk);
			clk_disable_unprepare(clk_ctx->bitclk);
		}
		return 0;
	}

	priv = dpu->crtc.dev->dev_private;
	hwdev = priv->hwdev;

	if (hwdev->is_hdmi) {
		clk_disable_unprepare(clk_ctx->hmclk);
	} else {
		clk_disable_unprepare(clk_ctx->pxclk);
		clk_disable_unprepare(clk_ctx->mclk);
		clk_disable_unprepare(clk_ctx->hclk);
		clk_disable_unprepare(clk_ctx->escclk);
		clk_disable_unprepare(clk_ctx->bitclk);
	}

	return 0;
}

u8 ky_plane_hw_get_format_id(u32 format)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(primary_fmts); i++) {
		if (primary_fmts[i].format == format)
			return primary_fmts[i].id;
	}

	return KY_DPU_INVALID_FORMAT_ID;
}

static bool parsr_afbc_modifier(uint64_t modifier, uint8_t* tile_type, uint8_t* block_size, uint8_t* yuv_transform, uint8_t* split_mode)
{
	uint64_t super_block_size = modifier & AFBC_FORMAT_MOD_BLOCK_SIZE_MASK;

	if (block_size && (super_block_size == AFBC_FORMAT_MOD_BLOCK_SIZE_16x16)) {
		*block_size = 0;
	} else if (block_size && (super_block_size == AFBC_FORMAT_MOD_BLOCK_SIZE_32x8)) {
		*block_size = 1;
	} else {
		DRM_ERROR("unsupport modifier = 0x%llu, super_block_size = 0x%llu, dpu support only 16x16 and 32x8!\n", modifier, super_block_size);
		return false;
	}

	if (tile_type && (modifier & AFBC_FORMAT_MOD_TILED)) {
		*tile_type = 1;
	}

	if (yuv_transform && (modifier & AFBC_FORMAT_MOD_YTR)) {
		*yuv_transform = 1;
	}

	if (split_mode && (modifier & AFBC_FORMAT_MOD_SPLIT)) {
		*split_mode = 1;
	}

	return true;
}

void ky_update_csc_matrix(struct drm_plane *plane, struct drm_plane_state *old_state){
	struct ky_plane_state *ky_plane_state = to_ky_plane_state(plane->state);
	u32 rdma_id = ky_plane_state->rdma_id;
	u32 module_base;
	int color_encoding = plane->state->color_encoding;
	int color_range = plane->state->color_range;
	struct ky_drm_private *priv = plane->dev->dev_private;
	int value;

	module_base = RDMA0_BASE_ADDR + rdma_id * RDMA_SIZE;

	if ((color_encoding != old_state->color_encoding) || (color_range != old_state->color_range)){
		value = (ky_yuv2rgb_coefs[color_encoding][color_range][0] & 0x3FFF) | ((ky_yuv2rgb_coefs[color_encoding][color_range][1] & 0x3FFF) << 14);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, CSC_MATRIX0, value);
		value = (ky_yuv2rgb_coefs[color_encoding][color_range][2] & 0x3FFF) | ((ky_yuv2rgb_coefs[color_encoding][color_range][3] & 0x3FFF) << 14);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, CSC_MATRIX1, value);
		value = (ky_yuv2rgb_coefs[color_encoding][color_range][4] & 0x3FFF) | ((ky_yuv2rgb_coefs[color_encoding][color_range][5] & 0x3FFF) << 14);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, CSC_MATRIX2, value);
		value = (ky_yuv2rgb_coefs[color_encoding][color_range][6] & 0x3FFF) | ((ky_yuv2rgb_coefs[color_encoding][color_range][7] & 0x3FFF) << 14);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, CSC_MATRIX3, value);
		value = (ky_yuv2rgb_coefs[color_encoding][color_range][8] & 0x3FFF) | ((ky_yuv2rgb_coefs[color_encoding][color_range][9] & 0x3FFF) << 14);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, CSC_MATRIX4, value);
		value = (ky_yuv2rgb_coefs[color_encoding][color_range][10] & 0x3FFF) | ((ky_yuv2rgb_coefs[color_encoding][color_range][11] & 0x3FFF) << 14);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, CSC_MATRIX5, value);
	}
}

static void saturn_conf_scaler_x(struct drm_plane_state *state)
{
	struct ky_plane_state *ky_plane_state = to_ky_plane_state(state);
	struct drm_plane *plane = state->plane;
	struct ky_drm_private *priv = plane->dev->dev_private;
	u32 in_width, in_height, out_width, out_height;
	uint32_t hor_delta_phase, ver_delta_phase;
	int64_t hor_init_phase, ver_init_phase;
	u32 module_base;

	trace_saturn_conf_scaler_x(ky_plane_state);

	/* should never happen */
	if (unlikely(ky_plane_state->scaler_id >= MAX_SCALER_NUMS))
		panic("Invalid scaler id:%d\n", ky_plane_state->scaler_id);

	if (state->rotation == DRM_MODE_ROTATE_90 || state->rotation == DRM_MODE_ROTATE_270) {
		in_width  = state->src_h >> 16;
		in_height = state->src_w >> 16;
	} else {
		in_width  = state->src_w >> 16;
		in_height = state->src_h >> 16;
	}

	out_width = state->crtc_w;
	out_height = state->crtc_h;

	/* Config SCALER scaling regs */
	module_base = SCALER0_ONLINE_BASE_ADDR + ky_plane_state->scaler_id * SCALER_SIZE;

	hor_delta_phase = in_width * 65536 / out_width;
	ver_delta_phase = in_height * 65536 / out_height;

	write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_7, hor_delta_phase); //0x10000
	write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_8, ver_delta_phase); //0x5555

	//TODO: start cor
	hor_init_phase = ((int64_t)hor_delta_phase - 65536) >> 1;
	ver_init_phase = ((int64_t)ver_delta_phase - 65536) >> 1;

	DRM_DEBUG("hor_delta:%d ver_delta:%d\n", hor_delta_phase, ver_delta_phase);
	DRM_DEBUG("hor_init_phase:%lld ver_init_phase:%lld\n", hor_init_phase, ver_init_phase);

	if (hor_init_phase >= 0) {
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_3, hor_init_phase & 0x00000000ffffffff);  //0x0
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_4, 0);
	} else { //convert to positive value if negative
		hor_init_phase += 8589934592;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_3, hor_init_phase & 0x00000000ffffffff);
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_4, (hor_init_phase & 0x0000000100000000) >> 32);
	}

	if (ver_init_phase >= 0) {
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_5, ver_init_phase & 0x00000000ffffffff);
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_6, 0);
	} else { //convert to positive value if negative
		ver_init_phase += 8589934592;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_5, ver_init_phase & 0x00000000ffffffff);
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_6, (ver_init_phase & 0x0000000100000000) >> 32);
	}

	// enable both ver and hor
	write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_0, in_width << 8 | 0x1 << 1 | 0x1);
	write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_1, out_width << 16 | in_height);
	write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_2, out_height);

	/* Config RDMA scaling regs */
	module_base = RDMA0_BASE_ADDR + ky_plane_state->rdma_id * RDMA_SIZE;

	ver_init_phase = ((int64_t)ver_delta_phase - 65536) / 2;
	if (ver_init_phase >= 0) {
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, LEFT_INIT_PHASE_V_LOW, ver_init_phase & 0x00000000ffffffff);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, LEFT_INIT_PHASE_V_HIGH, 0);  //TODO: only 1 bit, read then write??
	} else { //convert to positive value if negative
		ver_init_phase += 8589934592;
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, LEFT_INIT_PHASE_V_LOW, ver_init_phase & 0x00000000ffffffff);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, LEFT_INIT_PHASE_V_HIGH, (ver_init_phase & 0x0000000100000000) >> 32);
	}

	write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, LEFT_SCL_RATIO_V, 0x1 << 20 | ver_delta_phase);
}

void saturn_conf_scaler_coefs(struct drm_plane *plane, struct ky_plane_state *ky_pstate){
	struct drm_property_blob * blob = ky_pstate->scale_coefs_blob_prop;
	struct ky_drm_private *priv = plane->dev->dev_private;
	int scale_num = 192;
	u32 module_base;
	int val;
	int *coef_data;

	/* should never happen */
	if (unlikely(ky_pstate->scaler_id >= MAX_SCALER_NUMS))
		DRM_ERROR("Invalid scaler id:%d\n", ky_pstate->scaler_id);

	/* Config SCALER scaling regs */
	module_base = SCALER0_ONLINE_BASE_ADDR + ky_pstate->scaler_id * SCALER_SIZE;

	if (blob){
		/* should never happen */
		if (unlikely(blob->length != scale_num))
			DRM_ERROR("The blob length %ld is not correct scale_num %d\n", blob->length, scale_num);

		coef_data = (int *)blob->data;
		if (unlikely(NULL == coef_data)){
			DRM_ERROR("The coef data is NULL\n");
			return;
		}

		val  = (coef_data[0] & 0xFFFF) | coef_data[1] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_9, val);
		val  = (coef_data[2] & 0xFFFF) | coef_data[3] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_10, val);
		val  = (coef_data[4] & 0xFFFF) | coef_data[5] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_11, val);
		val  = (coef_data[6] & 0xFFFF) | coef_data[7] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_12, val);
		val  = (coef_data[8] & 0xFFFF) | coef_data[9] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_13, val);
		val  = (coef_data[10] & 0xFFFF) | coef_data[11] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_14, val);
		val  = (coef_data[12] & 0xFFFF) | coef_data[13] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_15, val);
		val  = (coef_data[14] & 0xFFFF) | coef_data[15] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_16, val);
		val  = (coef_data[16] & 0xFFFF) | coef_data[17] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_17, val);
		val  = (coef_data[18] & 0xFFFF) | coef_data[19] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_18, val);
		val  = (coef_data[20] & 0xFFFF) | coef_data[21] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_19, val);
		val  = (coef_data[22] & 0xFFFF) | coef_data[23] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_20, val);
		val  = (coef_data[24] & 0xFFFF) | coef_data[25] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_21, val);
		val  = (coef_data[26] & 0xFFFF) | coef_data[27] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_22, val);
		val  = (coef_data[28] & 0xFFFF) | coef_data[29] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_23, val);
		val  = (coef_data[30] & 0xFFFF) | coef_data[31] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_24, val);
		val  = (coef_data[32] & 0xFFFF) | coef_data[33] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_25, val);
		val  = (coef_data[34] & 0xFFFF) | coef_data[35] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_26, val);
		val  = (coef_data[36] & 0xFFFF) | coef_data[37] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_27, val);
		val  = (coef_data[38] & 0xFFFF) | coef_data[39] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_28, val);
		val  = (coef_data[40] & 0xFFFF) | coef_data[41] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_29, val);
		val  = (coef_data[42] & 0xFFFF) | coef_data[43] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_30, val);
		val  = (coef_data[44] & 0xFFFF) | coef_data[45] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_31, val);
		val  = (coef_data[46] & 0xFFFF) | coef_data[47] << 16;
		write_to_cmdlist(priv, SCALER_X_REG, module_base, disp_scl_reg_32, val);
	}
}

static void ky_set_afbc_info(struct ky_drm_private *priv, uint64_t modifier, u32 module_base)
{
	bool ret = false;
	u8 tile_type = 0;
	u8 block_size = 0;
	u8 yuv_transform = 0;
	u8 split_mode = 0;

	ret = parsr_afbc_modifier(modifier, &tile_type, &block_size, &yuv_transform, &split_mode);
	if (!ret) {
		DRM_ERROR("Faild to set afbc plane\n");
		return;
	}

	write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, AFBC_CFG, tile_type << 3 | block_size << 2 | yuv_transform << 1 | split_mode);

	return;
}

#define CONFIG_HW_COMPOSER_LAYER(id) \
{\
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _rect_ltopx, crtc_x); \
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _rect_ltopy, crtc_y); \
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _rect_rbotx, crtc_x + crtc_w - 1); \
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _rect_rboty, crtc_y + crtc_h - 1); \
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _color_key_en, 0); \
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _blend_mode, blend_mode); \
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _alpha_sel, alpha_sel); \
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _layer_alpha, alpha); \
	if (unlikely(solid_en)) { \
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _solid_color_R, solid_r); \
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _solid_color_G, solid_g); \
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _solid_color_B, solid_b); \
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _solid_color_A, solid_a); \
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _solid_en, 1); \
		DRM_DEBUG("solid_r:0x%x, solid_g:0x%x, solid_b:0x%x, solid_a:0x%x\n", \
			solid_r, solid_g, solid_b, solid_a); \
	} else { \
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _solid_en, 0); \
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _layer_id, rdma_id); \
	} \
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl ## id ## _en, 1); \
}

void ky_plane_update_hw_channel(struct drm_plane *plane)
{
	struct drm_plane_state *state = plane->state;
	struct ky_plane *ky_plane = to_ky_plane(plane);
	struct drm_framebuffer *fb = plane->state->fb;
	struct ky_plane_state *ky_plane_state = to_ky_plane_state(state);
	u32 rdma_id = ky_plane_state->rdma_id;
	u8 alpha = state->alpha >> 8;
	u16 pixel_alpha = state->pixel_blend_mode;
	u32 src_w, src_h, src_x, src_y;
	u32 crtc_w, crtc_h, crtc_x, crtc_y;
	u32 alpha_sel, blend_mode = 0;
	u8 uv_swap = 0;
	uint32_t fccf = fb->format->format;
	u32 module_base, base;
	u32 val;
	bool solid_en = false;
	u32 solid_a, solid_r, solid_g, solid_b;
	struct ky_drm_private *priv = plane->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	bool is_afbc = (fb->modifier > 0);
	u8 channel = crtc_to_dpu(state->crtc)->dev_id;

	trace_ky_plane_update_hw_channel("rdma_id", rdma_id);

	src_w = state->src_w >> 16;
	src_h = state->src_h >> 16;
	src_x = state->src_x >> 16;
	src_y = state->src_y >> 16;

	crtc_w = state->crtc_w;
	crtc_h = state->crtc_h;
	crtc_x = state->crtc_x;
	crtc_y = state->crtc_y;

	if (ky_plane_state->is_crop) {

		// src_w = state->src_w >> 16;
		src_h = state->src_h >> 16;
		src_x = state->src_x >> 16;
		src_y = state->src_y >> 16;

		// crtc_w = state->crtc_w;
		crtc_h = state->crtc_h;
		// crtc_x = state->crtc_x;
		// crtc_y = state->crtc_y;

		src_w = ky_plane_state->src_crop_w;
		crtc_w = ky_plane_state->dst_crop_w;

		crtc_x = ky_plane_state->dst_crop_x;
		crtc_y = ky_plane_state->dst_crop_y;

	}

	if (rdma_id == RDMA_INVALID_ID)
		solid_en = true;

	ky_plane_state->is_offline = 0;

	trace_dpu_plane_info(state, fb, rdma_id, alpha,
			     state->rotation, is_afbc);

	/* For solid color both src_w and src_h are 0 */
	if (solid_en == false) {
		/* Set RDMA regs */
		module_base = RDMA0_BASE_ADDR + rdma_id * RDMA_SIZE;
		val = 0x0 << 26 | 0x0 << 25 | 0xf << 17 | 0x0 << 15 | channel << 12 | 0x0 << 7 | 0xf << 2 | (is_afbc ? 1 : 0);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, LAYER_CTRL, val);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, COMPSR_Y_OFST, crtc_y);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, LEFT_SCL_RATIO_V, 0x0 << 20);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, LEFT_IMG_SIZE, fb->height << 16 | fb->width);
		val = src_y + src_h;

		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, LEFT_CROP_POS_START, src_y << 16 | src_x);
		val = ((val - 1) << 16) | (src_x + src_w - 1);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, LEFT_CROP_POS_END, val);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, LEFT_ALPHA01, 0x0 << 16 | 0x0);
		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, LEFT_ALPHA23, 0x0 << 16 | 0x0);

		saturn_write_fbcmem_regs(state, rdma_id, module_base, NULL);

		val = 0;
		/* setup the rotation and axis flip bits */
		if (state->rotation & DRM_MODE_ROTATE_MASK)
			val = ilog2(plane->state->rotation & DRM_MODE_ROTATE_MASK) & 0x3;
		if (state->rotation & DRM_MODE_REFLECT_MASK)
			val = ilog2(plane->state->rotation & DRM_MODE_REFLECT_MASK);

		if (fccf == DRM_FORMAT_YVYU || fccf == DRM_FORMAT_VYUY || fccf == DRM_FORMAT_NV21 || fccf == DRM_FORMAT_YVU420 || fccf == DRM_FORMAT_Q401) {
			uv_swap = 1;
		}

		write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, ROT_MODE, val << 7 | uv_swap << 6 | ky_plane_state->format);

		if (fb->modifier) {
			ky_set_afbc_info(priv, fb->modifier, module_base);
		}

		if (ky_plane_state->use_scl){
			saturn_conf_scaler_x(state);

			/*scaling mismatch gpu render result without these coefs*/
			saturn_conf_scaler_coefs(plane, ky_plane_state);
		}
	} else {
		solid_r = (u8)ky_plane_state->solid_color;
		solid_g = (u8)(ky_plane_state->solid_color >> 8);
		solid_b = (u8)(ky_plane_state->solid_color >> 16);
		solid_a = (u8)(ky_plane_state->solid_color >> 24);
		solid_r = solid_r << hwdev->solid_color_shift;
		solid_g = solid_g << hwdev->solid_color_shift;
		solid_b = solid_b << hwdev->solid_color_shift;
	}

	/* Set layer regs */
	if (state->fb->format->has_alpha)
		alpha_sel = 0x1;
	else
		alpha_sel = 0x0;

	switch (pixel_alpha) {
	case DRM_MODE_BLEND_PIXEL_NONE:
		/* x1 supports knone + layer alpha*/
		alpha_sel = 0x0;
		fallthrough;
	case DRM_MODE_BLEND_COVERAGE:
		blend_mode = 0x0;
		break;
	case DRM_MODE_BLEND_PREMULTI:
		blend_mode = 0x1;
		break;
	default:
		DRM_ERROR("Unsupported blend mode!\n");
	}

	/* enable composer and bind RDMA */
	base = CMP_BASE_ADDR(channel);
	switch(ky_plane->hw_pid) {
	case 0:
		CONFIG_HW_COMPOSER_LAYER(0);
		break;
	case 1:
		CONFIG_HW_COMPOSER_LAYER(1);
		break;
	case 2:
		CONFIG_HW_COMPOSER_LAYER(2);
		break;
	case 3:
		CONFIG_HW_COMPOSER_LAYER(3);
		break;
	case 4:
		CONFIG_HW_COMPOSER_LAYER(4);
		break;
	case 5:
		CONFIG_HW_COMPOSER_LAYER(5);
		break;
	case 6:
		CONFIG_HW_COMPOSER_LAYER(6);
		break;
	case 7:
		CONFIG_HW_COMPOSER_LAYER(7);
		break;
	default:
		DRM_ERROR("%s unsupported zpos:%d\n", __func__, state->zpos);
		break;
	}
}

void ky_plane_disable_hw_channel(struct drm_plane *plane, struct drm_plane_state *old_state)
{
	struct ky_plane *p = to_ky_plane(plane);
	u8 channel = crtc_to_dpu(old_state->crtc)->dev_id;
	u32 base = CMP_BASE_ADDR(channel);
	u32 rdma_id = to_ky_plane_state(old_state)->rdma_id;
	struct ky_drm_private *priv = plane->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;

	DRM_DEBUG("%s() layer_id = %u rdma_id:%d\n", __func__, p->hw_pid, rdma_id);

	trace_ky_plane_disable_hw_channel(p->hw_pid, rdma_id);

	switch (p->hw_pid) {
	case 0:
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl0_en, 0);
		break;
	case 1:
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl1_en, 0);
		break;
	case 2:
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl2_en, 0);
		break;
	case 3:
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl3_en, 0);
		break;
	case 4:
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl4_en, 0);
		break;
	case 5:
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl5_en, 0);
		break;
	case 6:
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl6_en, 0);
		break;
	case 7:
		dpu_write_reg(hwdev, CMPS_X_REG, base, m_nl7_en, 0);
		break;
	default:
		DRM_ERROR("%s unsupported zpos:%d\n", __func__, old_state->zpos);
		break;
	}
}

static u32 saturn_conf_dpuctrl_scaling(struct ky_dpu *dpu)
{
	struct ky_dpu_scaler *scaler = NULL;
	struct ky_crtc_state *ac = to_ky_crtc_state(dpu->crtc.state);
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	u32 scl_en = 0;
	u32 i;

	for (i = 0; i < MAX_SCALER_NUMS; i++) {
		scaler = &(ac->scalers[i]);
		DRM_DEBUG("scaler%d: in_use:0x%x rdma_id:%d\n", i, scaler->in_use, scaler->rdma_id);
		trace_dpuctrl_scaling_setting(i, scaler->in_use, scaler->rdma_id);
		if (scaler->in_use) {
			scl_en |= (i + 1);
			//TODO: expand below code when more than 2 sclaers
			if (i == 0)
				dpu_write_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR, ctl_nml_scl0_layer_id, scaler->rdma_id);
			if (i == 1)
				dpu_write_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR, ctl_nml_scl1_layer_id, scaler->rdma_id);
		}
	}

	DRM_DEBUG("scl_en:%d\n", scl_en);
	trace_dpuctrl_scaling("scl_en", scl_en);
	return scl_en;
}

void ky_update_hdr_matrix(struct drm_plane *plane, struct ky_plane_state *ky_pstate){

}

static u32 saturn_conf_dpuctrl_rdma(struct ky_dpu *dpu)
{
	struct drm_crtc *crtc = &dpu->crtc;
	struct drm_plane *plane;
	u32 rdma_en = 0;

	/* Find out the active rdmas */
	drm_atomic_crtc_for_each_plane(plane, crtc) {
		u32 rdma_id = to_ky_plane_state(plane->state)->rdma_id;

		if (rdma_id != RDMA_INVALID_ID)
			rdma_en |= (1 << rdma_id);
	}

	trace_dpuctrl("rdma_en", rdma_en);

	return rdma_en;
}

void saturn_conf_dpuctrl_color_matrix(struct ky_dpu *dpu, struct drm_crtc_state *old_state)
{
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	struct drm_crtc_state *state = dpu->crtc.state;
	struct ky_crtc_state *ky_state = to_ky_crtc_state(state);
	struct drm_property_blob * blob = ky_state->color_matrix_blob_prop;
	int *color_matrix;

	/*For color matrix, if no update from user space,
	we keep the original configuration, do not change the value of any color matix register*/
	if (blob){
		color_matrix = (int*)blob->data;

		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_npost_proc_en, 1);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_nendmatrix_en, 1);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_ngain_to_full_en, 0);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_nmatrix_en, 0);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_nfront_tmootf_en, 0);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_nend_tmootf_en, 0);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_neotf_en, 0);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_noetf_en, 0);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_pendmatrix_table0, color_matrix[0]);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_pendmatrix_table1, color_matrix[1]);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_pendmatrix_table2, color_matrix[2]);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_pendmatrix_table3, color_matrix[3]);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_pendmatrix_table4, color_matrix[4]);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_pendmatrix_table5, color_matrix[5]);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_pendmatrix_table6, color_matrix[6]);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_pendmatrix_table7, color_matrix[7]);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_pendmatrix_table8, color_matrix[8]);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_pendmatrix_offset0, color_matrix[9]);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_pendmatrix_offset1, color_matrix[10]);
		dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_pendmatrix_offset2, color_matrix[11]);
	}

	return;
}

static void saturn_conf_dpuctrl(struct drm_crtc *crtc,
		    struct drm_crtc_state *old_state)
{
	u32 scl_en = 0;
	u32 rdma_en = 0;
	u32 base = DPU_CTRL_BASE_ADDR;
	struct ky_drm_private *priv = crtc->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	struct ky_dpu *dpu = crtc_to_dpu(crtc);

	rdma_en = saturn_conf_dpuctrl_rdma(dpu);
	scl_en = saturn_conf_dpuctrl_scaling(dpu);

	cmdlist_sort_by_group(crtc);
	cmdlist_atomic_commit(crtc, old_state);

	switch (dpu->dev_id) {
	case ONLINE2:
		dpu_write_reg(hwdev, DPU_CTL_REG, base, ctl2_nml_scl_en, scl_en);
		dpu_write_reg(hwdev, DPU_CTL_REG, base, ctl2_nml_rch_en, rdma_en);
		dpu_write_reg(hwdev, DPU_CTL_REG, base, ctl2_nml_outctl_en, 0x1);
		break;
	case OFFLINE0:
		/* TODO */
		break;
	default:
		DRM_ERROR("%s, dpu_id %d is invalid!\n", __func__, dpu->dev_id);
		break;
	}
}

static void saturn_irq_enable(struct ky_dpu *dpu, bool enable)
{
	u32 base = DPU_INT_BASE_ADDR;
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;

	trace_saturn_irq_enable("irq", enable);

	if (dpu->dev_id == ONLINE2) {
		/* enable online2 irq */
		dpu_write_reg(hwdev, DPU_INTP_REG, base, b.onl2_nml_frm_timing_eof_int_msk, enable ? 1 : 0);
		dpu_write_reg(hwdev, DPU_INTP_REG, base, b.onl2_nml_cfg_rdy_clr_int_msk, enable ? 1 : 0);
		dpu_write_reg(hwdev, DPU_INTP_REG, base, b.onl2_nml_frm_timing_unflow_int_msk, enable ? 1 : 0);
		dpu_write_reg(hwdev, DPU_INTP_REG, base, b.onl2_nml_cmdlist_ch_frm_cfg_done_int_msk, enable ? 0xF : 0);
	}
}

static void saturn_enable_vsync(struct ky_dpu *dpu, bool enable)
{
	u32 base = DPU_INT_BASE_ADDR;
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;

	trace_saturn_enable_vsync("vsync", enable);
	if (dpu->dev_id == ONLINE2)
		dpu_write_reg(hwdev, DPU_INTP_REG, base, b.onl2_nml_frm_timing_vsync_int_msk, enable ? 1 : 0);
}

static void saturn_ctrl_cfg_ready(struct ky_dpu *dpu, bool enable)
{
	u32 base = DPU_CTRL_BASE_ADDR;
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;

	trace_saturn_ctrl_cfg_ready(dpu->dev_id, enable);

	switch (dpu->dev_id) {
	case ONLINE0:
		dpu_write_reg(hwdev, DPU_CTL_REG, base, ctl0_nml_cfg_rdy, enable ? 1 : 0);
		break;
	case ONLINE1:
		dpu_write_reg(hwdev, DPU_CTL_REG, base, ctl1_nml_cfg_rdy, enable ? 1 : 0);
		break;
	case ONLINE2:
		dpu_write_reg(hwdev, DPU_CTL_REG, base, ctl2_nml_cfg_rdy, enable ? 1 : 0);
		break;
	case OFFLINE0:
		dpu_write_reg(hwdev, DPU_CTL_REG, base, ctl3_nml_cfg_rdy, enable ? 1 : 0);
		break;
	case OFFLINE1:
		dpu_write_reg(hwdev, DPU_CTL_REG, base, ctl4_nml_cfg_rdy, enable ? 1 : 0);
		break;
	default:
		DRM_ERROR("id is invalid!\n");
		break;
	}
}

static void saturn_ctrl_sw_start(struct ky_dpu *dpu, bool enable)
{
	u32 base = DPU_CTRL_BASE_ADDR;
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;

	trace_saturn_ctrl_sw_start(dpu->dev_id, enable);

	switch (dpu->dev_id) {
	case ONLINE0:
		dpu_write_reg_w1c(hwdev, DPU_CTL_REG, base, ctl0_sw_start, enable ? 1 : 0);
		break;
	case ONLINE1:
		dpu_write_reg_w1c(hwdev, DPU_CTL_REG, base, ctl1_sw_start, enable ? 1 : 0);
		break;
	case ONLINE2:
		dpu_write_reg_w1c(hwdev, DPU_CTL_REG, base, ctl2_sw_start, enable ? 1 : 0);
		break;
	case OFFLINE0:
		dpu_write_reg_w1c(hwdev, DPU_CTL_REG, base, ctl3_sw_start, enable ? 1 : 0);
		break;
	default:
		DRM_ERROR("id is invalid!\n");
		break;
	}
}

void saturn_wb_disable(struct ky_dpu *dpu)
{
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;

	dpu_write_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR, ctl2_nml_wb_en, 0);
	dpu_write_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR, ctl2_nml_cfg_rdy, 1);
}

void saturn_wb_config(struct ky_dpu *dpu)
{

}

static u32 dpu_get_version(struct ky_dpu *dpu)
{
	return 0;
}

static void saturn_init_csc(struct ky_dpu *dpu){
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	u32 module_base;
	int i = 0;

	for(i = 0; i < hwdev->rdma_nums; i++){
		module_base = RDMA0_BASE_ADDR + i * RDMA_SIZE;
		dpu_write_reg(hwdev, RDMA_PATH_X_REG, module_base, b.csc_matrix00, ky_yuv2rgb_coefs[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE][0] & 0x3FFF);
		dpu_write_reg(hwdev, RDMA_PATH_X_REG, module_base, b.csc_matrix01, ky_yuv2rgb_coefs[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE][1] & 0x3FFF);
		dpu_write_reg(hwdev, RDMA_PATH_X_REG, module_base, b.csc_matrix02, ky_yuv2rgb_coefs[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE][2] & 0x3FFF);
		dpu_write_reg(hwdev, RDMA_PATH_X_REG, module_base, b.csc_matrix03, ky_yuv2rgb_coefs[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE][3] & 0x3FFF);
		dpu_write_reg(hwdev, RDMA_PATH_X_REG, module_base, b.csc_matrix10, ky_yuv2rgb_coefs[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE][4] & 0x3FFF);
		dpu_write_reg(hwdev, RDMA_PATH_X_REG, module_base, b.csc_matrix11, ky_yuv2rgb_coefs[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE][5] & 0x3FFF);
		dpu_write_reg(hwdev, RDMA_PATH_X_REG, module_base, b.csc_matrix12, ky_yuv2rgb_coefs[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE][6] & 0x3FFF);
		dpu_write_reg(hwdev, RDMA_PATH_X_REG, module_base, b.csc_matrix13, ky_yuv2rgb_coefs[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE][7] & 0x3FFF);
		dpu_write_reg(hwdev, RDMA_PATH_X_REG, module_base, b.csc_matrix20, ky_yuv2rgb_coefs[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE][8] & 0x3FFF);
		dpu_write_reg(hwdev, RDMA_PATH_X_REG, module_base, b.csc_matrix21, ky_yuv2rgb_coefs[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE][9] & 0x3FFF);
		dpu_write_reg(hwdev, RDMA_PATH_X_REG, module_base, b.csc_matrix22, ky_yuv2rgb_coefs[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE][10] & 0x3FFF);
		dpu_write_reg(hwdev, RDMA_PATH_X_REG, module_base, b.csc_matrix23, ky_yuv2rgb_coefs[DRM_COLOR_YCBCR_BT709][DRM_COLOR_YCBCR_LIMITED_RANGE][11] & 0x3FFF);
	}
}

static void saturn_init_regs(struct ky_dpu *dpu)
{
	u32 base = 0;
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	struct drm_crtc *crtc = &dpu->crtc;
	struct drm_display_mode *mode = &crtc->mode;
	u8 channel = dpu->dev_id;
	u16 vfp, vbp, vsync, hfp, hbp, hsync;
	// u32 value;

	hsync = mode->hsync_end - mode->hsync_start;
	hbp = mode->htotal - mode->hsync_end;
	hfp = mode->hsync_start - mode->hdisplay;
	vsync = mode->vsync_end - mode->vsync_start;
	vbp = mode->vtotal - mode->vsync_end;
	vfp = mode->vsync_start - mode->vdisplay;
	trace_drm_display_mode_info(mode);

	base = CMP_BASE_ADDR(channel);
	/* set bg color to black */
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nbg_color_B, 0x0);
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nbg_color_R, 0x0);
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nbg_color_G, 0xFF);
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_nbg_color_A, 0xFF);
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_ncmps_en, 1);

	dpu_write_reg(hwdev, CMPS_X_REG, base, m_noutput_width, mode->hdisplay);
	dpu_write_reg(hwdev, CMPS_X_REG, base, m_noutput_height, mode->vdisplay);

	base = OUTCTRL_BASE_ADDR(channel);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, m_n_inheight, mode->vdisplay);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, m_n_inwdith, mode->hdisplay);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, scale_en, 0);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, dither_en, 1);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, sbs_en, 0);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, split_en, 0);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, narrow_yuv_en, 0);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, cmd_screen, 0);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, frame_timing_en, 1);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, split_overlap, 0);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, hblank, 0);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, back_ground_r, 0xfff);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, back_ground_g, 0x0);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, back_ground_b, 0x0);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, sof_pre_ln_num, 0x0);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, cfg_ln_num_intp, 0x0);

	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, hbp, hbp);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, hfp, hfp);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, vfp, vfp);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, vbp, vbp);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, hsync_width, hsync);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, hsp, 1);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, vsync_width, vsync);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, vsp, 1);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, h_active, mode->hdisplay);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, v_active, mode->vdisplay);

	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, user, 2); /* RGB888 */

	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, eof_ln_dly, 16);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, eof0_irq_mask, 1);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, underflow0_irq_mask, 1);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, eof1_irq_mask, 1);
	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, underflow1_irq_mask, 1);

	// if (hwdev->is_hdmi) {
	// 	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, base, disp_ready_man_en, 1);
	// 	value = dpu_read_reg(hwdev, OUTCTRL_TOP_X_REG, base, value32[31]);
	// 	DRM_INFO("%s read OUTCTRL_TOP_X_REG value32[31] 0x%x", __func__, value);
	// }

	dpu_write_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR, ctl2_video_mod, 0x1);
	dpu_write_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR, ctl2_dbg_mod, 0x0);
	/*
	 * ctl2_timing_inter0 use default value
	 * ctl2_timing_inter1 = 2 * ⌈fmclk / fdscclk⌉, set 0xf as max value
	 */
	dpu_write_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR, ctl2_timing_inter1, DPU_CTRL_MAX_TIMING_INTER1);

	dpu_write_reg(hwdev, MMU_REG, MMU_BASE_ADDR, b.cfg_dmac0_rd_outs_num, 16);
	dpu_write_reg(hwdev, MMU_REG, MMU_BASE_ADDR, b.cfg_dmac0_axi_burst, 1);

	/* totally disable post proc */
	dpu_write_reg(hwdev, OUTCTRL_PROC_X_REG, PP2_BASE_ADDR, m_npost_proc_en, 0);
}

static void saturn_setup_dma_top(struct ky_dpu *dpu)
{
	u32 base = DMA_TOP_BASE_ADDR;
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;

	dpu_write_reg(hwdev, DMA_TOP_REG, base, img_rr_ratio, 0x10);
	dpu_write_reg(hwdev, DMA_TOP_REG, base, round_robin_mode, 0);
	dpu_write_reg(hwdev, DMA_TOP_REG, base, pixel_num_th, 4);

	//regnum: 2, offset: 0x08
	dpu_write_reg(hwdev, DMA_TOP_REG, base, rdma_timeout_limit, 0xFFFE);
	dpu_write_reg(hwdev, DMA_TOP_REG, base, wdma_timeout_limit, 0xFFFF);

	//regnum: 7, offset: 0x1C
	dpu_write_reg(hwdev, DMA_TOP_REG, base, online_rqos, DPU_QOS_NORMAL);
	dpu_write_reg(hwdev, DMA_TOP_REG, base, offline_rqos, DPU_QOS_LOW);
	dpu_write_reg(hwdev, DMA_TOP_REG, base, online_wqos, DPU_QOS_LOW);
	dpu_write_reg(hwdev, DMA_TOP_REG, base, offline_wqos, DPU_QOS_LOW);

	//regnum: 17, offset: 0x44
	dpu_write_reg(hwdev, DMA_TOP_REG, base, cmdlist_rqos, 4);
	dpu_write_reg(hwdev, DMA_TOP_REG, base, dmac0_burst_length, 1);

	dpu_write_reg(hwdev, DMA_TOP_REG, base, dmac0_rd_outs_num, 8);
	dpu_write_reg(hwdev, DMA_TOP_REG, base, dmac0_wr_outs_num, 16);

	if (dpu_read_reg(hwdev, DMA_TOP_REG, base, cmdlist_rqos) < DPU_QOS_URGENT || \
	    dpu_read_reg(hwdev, DMA_TOP_REG, base, online_rqos) < dpu_read_reg(hwdev, DMA_TOP_REG, base, offline_rqos)) {
		// panic("dma_top qos cfg failed!\n");
	}
}


static void saturn_setup_mmu_top(struct ky_dpu *dpu)
{
	unsigned int val;
	unsigned int rd_outs_num = 0;
	u32 base = MMU_BASE_ADDR;
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;

	rd_outs_num = RD_OUTS_NUM / 2;

	/* MMU TOP init setting */
	dpu_write_reg(hwdev, MMU_REG, base, v.TBU_Timelimit, (0x1 << 16) | RDMA_TIMELIMIT);
	dpu_write_reg(hwdev, MMU_REG, base, v.TBU_AXI_PORT_SEL, 0);

	val = dpu_read_reg(hwdev, MMU_REG, base, v.MMU_Dmac0_Reg);
	val &= ~(0xFF << 16);
	val |= (rd_outs_num << 16);
	dpu_write_reg(hwdev, MMU_REG, base, v.MMU_Dmac0_Reg, val);

	val = dpu_read_reg(hwdev, MMU_REG, base, v.MMU_Dmac1_Reg);
	val &= ~(0xFF << 16);
	val |= (rd_outs_num << 16);
	dpu_write_reg(hwdev, MMU_REG, base, v.MMU_Dmac1_Reg, val);

	val = dpu_read_reg(hwdev, MMU_REG, base, v.MMU_Dmac2_Reg);
	val &= ~(0xFF << 16);
	val |= (rd_outs_num << 16);
	dpu_write_reg(hwdev, MMU_REG, base, v.MMU_Dmac2_Reg, val);

	val = dpu_read_reg(hwdev, MMU_REG, base, v.MMU_Dmac3_Reg);
	val &= ~(0xFF << 16);
	val |= (rd_outs_num << 16);
	dpu_write_reg(hwdev, MMU_REG, base, v.MMU_Dmac3_Reg, val);
}

static int dpu_init(struct ky_dpu *dpu)
{
	unsigned int timeout = 1000;
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
#ifdef CONFIG_KY_FPGA
	void __iomem *addr = (void __iomem *)ioremap(0xD4282800, 100);
#endif
	void __iomem *ciu_addr = (void __iomem *)ioremap(0xD4282C00, 0x200);
	u32 value;

	DRM_INFO("%s \n", __func__);
	trace_dpu_init(dpu->dev_id);

	while(timeout) {
		if (dpu_read_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR, value32[27]) == 0)
			break;
		udelay(100);
		timeout--;
	};
	if (timeout == 0)
		DRM_ERROR("%s wait cfg ready done timeout\n", __func__);

#ifdef CONFIG_KY_FPGA
	// for FPGA: enable PMU
	writel(0xffa1ffff, addr + 0x44);
	writel(0xFF65FF05, addr + 0x4c);
#endif

	// modified hdmi and mipi dsi qos
	value = readl_relaxed(ciu_addr + 0x011c);
	DRM_DEBUG("%s ciu offset 0x011c:0x%x\n", __func__, value);
	value = readl_relaxed(ciu_addr + 0x0124);
	DRM_DEBUG("%s ciu offset 0x0124:0x%x\n", __func__, value);
	writel(value | 0xffff, ciu_addr + 0x0124);
	udelay(2);
	value = readl_relaxed(ciu_addr + 0x0124);
	DRM_DEBUG("%s ciu offset 0x0124:0x%x\n", __func__, value);

	saturn_init_regs(dpu);
	saturn_setup_dma_top(dpu);
	saturn_setup_mmu_top(dpu);
	saturn_irq_enable(dpu, true);
	saturn_init_csc(dpu);

	return 0;
}

static void dpu_uninit(struct ky_dpu *dpu)
{
	trace_dpu_uninit(dpu->dev_id);
	saturn_irq_enable(dpu, false);
}

static uint32_t dpu_isr(struct ky_dpu *dpu)
{
	uint32_t irq_raw, int_mask = 0;
	u32 base = DPU_INT_BASE_ADDR;
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	static bool flip_done[DP_MAX_DEVICES] = {false};
	struct drm_writeback_connector *wb_conn = &dpu->wb_connector;
	u8 channel = dpu->dev_id;
	int flip_id;

	if (hwdev->is_hdmi) {
		flip_id = SATURN_HDMI;
	} else {
		flip_id = SATURN_LE;
	}

	trace_dpu_isr(dpu->dev_id);

	if (dpu->dev_id == ONLINE2) {
		irq_raw = dpu_read_reg(hwdev, DPU_INTP_REG, base, v.dpu_int_reg_24);
		trace_dpu_isr_status("ONLINE2", irq_raw );
		/* underrun */
		if (irq_raw & DPU_INT_FRM_TIMING_UNFLOW) {
			dpu_write_reg_w1c(hwdev, DPU_INTP_REG, base, v.dpu_int_reg_14, DPU_INT_FRM_TIMING_UNFLOW);
			trace_dpu_isr_status("Under Run!", irq_raw & DPU_INT_FRM_TIMING_UNFLOW);
			trace_dpu_isr_ul_data("DPU Mclk", dpu->cur_mclk);
			trace_dpu_isr_ul_data("DPU BW", dpu->cur_bw);
			dpu_dump_reg(dpu);
			queue_work(dpu->dpu_underrun_wq, &dpu->work_stop_trace);
			DRM_ERROR_RATELIMITED("Under Run! DPU_Mclk = %ld, DPU BW = %ld\n", (unsigned long)dpu->cur_mclk, (unsigned long)dpu->cur_bw);
			int_mask |= DPU_INT_UNDERRUN;
		}
		/* eof */
		if (irq_raw & DPU_INT_FRM_TIMING_EOF) {
			dpu_write_reg_w1c(hwdev, OUTCTRL_TOP_X_REG, OUTCTRL_BASE_ADDR(channel), eof0_irq_status, 1);
			dpu_write_reg_w1c(hwdev, OUTCTRL_TOP_X_REG, OUTCTRL_BASE_ADDR(channel), eof1_irq_status, 1);
			dpu_write_reg_w1c(hwdev, DPU_INTP_REG, base, v.dpu_int_reg_14, DPU_INT_FRM_TIMING_EOF);
			trace_dpu_isr_status("eof", irq_raw & DPU_INT_FRM_TIMING_EOF);
		}
		/* cfg ready clear */
		if (irq_raw & DPU_INT_CFG_RDY_CLR) {
			dpu_write_reg_w1c(hwdev, DPU_INTP_REG, base, v.dpu_int_reg_14, DPU_INT_CFG_RDY_CLR);
			trace_dpu_isr_status("cfg_rdy_clr", irq_raw & DPU_INT_CFG_RDY_CLR);
			flip_done[flip_id] = false;

			trace_u64_data("irq crtc mclk cur", dpu->cur_mclk);
			trace_u64_data("irq crtc mclk new", dpu->new_mclk);
			trace_u64_data("irq crtc bw cur", dpu->cur_bw);
			trace_u64_data("irq crtc bw new", dpu->new_bw);
			if (dpu->enable_auto_fc && dpu->new_mclk < dpu->cur_mclk) {
				trace_u64_data("run wq to set mclk", dpu->new_mclk);
				queue_work(system_wq, &dpu->work_update_clk);
			}
			if (dpu->enable_auto_fc && dpu->new_bw < dpu->cur_bw) {
				trace_u64_data("run wq to set bw", dpu->new_bw);
				queue_work(system_wq, &dpu->work_update_bw);
			}
		}
		/* vsync */
		if (irq_raw & DPU_INT_FRM_TIMING_VSYNC) {
			struct drm_crtc *crtc = &dpu->crtc;
			dpu_write_reg_w1c(hwdev, DPU_INTP_REG, base, v.dpu_int_reg_14, DPU_INT_FRM_TIMING_VSYNC);
			trace_u64_data("dpu name", (u64)hwdev->is_hdmi);
			trace_dpu_isr_status("vsync", irq_raw & DPU_INT_FRM_TIMING_VSYNC);
			drm_crtc_handle_vblank(crtc);

			if (!flip_done[flip_id]) {
				struct drm_device *drm = dpu->crtc.dev;
				struct drm_pending_vblank_event *event = crtc->state->event;

				flip_done[flip_id] = true;

				spin_lock(&drm->event_lock);
				if (crtc->state->event) {
					/*
					 * Set event to NULL first to ensure event is consumed
					 * before drm_atomic_helper_commit_hw_done.
					 */
					crtc->state->event = NULL;
					drm_crtc_send_vblank_event(crtc, event);
				}
				spin_unlock(&drm->event_lock);
			}
			dpu_dump_fps(dpu);
		}
		/* wb done */
		if (irq_raw & DPU_INT_WB_DONE) {
			dpu_write_reg_w1c(hwdev, DPU_INTP_REG, base, b.onl2_nml_wb_done_int_sts, irq_raw);
			dpu_write_reg_w1c(hwdev, DPU_INTP_REG, base, v.dpu_int_reg_14, DPU_INT_WB_DONE);
			saturn_wb_disable(dpu);
			drm_writeback_signal_completion(wb_conn, 0);
		}
		/* rest irq status */
		if (irq_raw & DPU_REST_INT_BITS)
			dpu_write_reg_w1c(hwdev, DPU_INTP_REG, base, v.dpu_int_reg_14, DPU_REST_INT_BITS);

	}
	if (dpu->dev_id == OFFLINE0) {
		/*cfg ready*/
		irq_raw = dpu_read_reg(hwdev, DPU_INTP_REG, base, b.offl0_cfg_rdy_clr_int_raw);
		if (irq_raw)
			dpu_write_reg_w1c(hwdev, DPU_INTP_REG, base, b.offl0_cfg_rdy_clr_int_sts, 1);

		/*wb*/
		irq_raw = dpu_read_reg(hwdev, DPU_INTP_REG, base, b.offl0_wb_frm_done_int_raw);
		if (irq_raw)
			dpu_write_reg_w1c(hwdev, DPU_INTP_REG, base, b.offl0_wb_frm_done_int_sts, irq_raw);
	}

	return int_mask;
}

static void dpu_run(struct drm_crtc *crtc,
		    struct drm_crtc_state *old_state)
{
	struct ky_dpu *dpu = crtc_to_dpu(crtc);
	trace_dpu_run(dpu->dev_id);

	/* config dpuctrl modules */
	saturn_conf_dpuctrl(crtc, old_state);

	//dsb(sy);
	mb();
	saturn_ctrl_cfg_ready(dpu, true);

	if (unlikely(dpu->is_1st_f)) {
		DRM_INFO("DPU type %d id %d Start!\n", dpu->type, dpu->dev_id);
		dpu->is_1st_f = false;
		saturn_ctrl_sw_start(dpu, true);
	}

#ifdef CONFIG_ARM64
	__iomb();
#else
	dma_rmb();
#endif
}

static void dpu_stop(struct ky_dpu *dpu)
{
	unsigned int timeout = DPU_STOP_TIMEOUT;
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	u8 channel = dpu->dev_id;

	flush_workqueue(dpu->dpu_underrun_wq);

	trace_dpu_stop(dpu->dev_id);

	while(timeout) {
		if (dpu_read_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR, value32[27]) == 0)
			break;
		udelay(10);
		timeout--;
	};
	if (timeout == 0)
		DRM_ERROR("%s wait cfg ready done timeout\n", __func__);
	else
		DRM_DEBUG("%s wait cfg ready done %d\n", __func__, timeout);

	dpu_write_reg(hwdev, OUTCTRL_TOP_X_REG, OUTCTRL_BASE_ADDR(channel), value32[6], 0x0);
	dpu_write_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR, ctl2_nml_rch_en, 0x0);
	dpu_write_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR, ctl2_nml_cfg_rdy, 0x1);

	timeout = DPU_STOP_TIMEOUT;
	while(timeout) {
		if (dpu_read_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR, value32[27]) & 1) {
			udelay(10);
			timeout--;
			continue;
		} else
			break;
	};
	if (timeout == 0)
		DRM_ERROR("%s stop timeout\n", __func__);
	else
		DRM_DEBUG("%s stop Done %d\n", __func__, timeout);
}

static int dpu_modeset(struct ky_dpu *dpu, struct drm_mode_modeinfo *mode)
{
	return 0;
}

static void dpu_enable_vsync(struct ky_dpu *dpu)
{
	saturn_enable_vsync(dpu, true);
}

static void dpu_disable_vsync(struct ky_dpu *dpu)
{
	saturn_enable_vsync(dpu, false);
}


static struct dpu_core_ops dpu_saturn_ops = {
	.parse_dt = dpu_parse_dt,
	.version = dpu_get_version,
	.init = dpu_init,
	.uninit = dpu_uninit,
	.run = dpu_run,
	.stop = dpu_stop,
	.isr = dpu_isr,
	.modeset = dpu_modeset,
	.enable_clk = dpu_enable_clocks,
	.disable_clk = dpu_disable_clocks,
	.update_clk = dpu_update_clocks,
	.update_bw = dpu_update_bw,
	.enable_vsync = dpu_enable_vsync,
	.disable_vsync = dpu_disable_vsync,
	.cal_layer_fbcmem_size = saturn_cal_layer_fbcmem_size,
	.calc_plane_mclk_bw = dpu_calc_plane_mclk_bw,
	.adjust_rdma_fbcmem = saturn_adjust_rdma_fbcmem,
	.wb_config = saturn_wb_config,
};

static struct ops_entry entry = {
	.ver = "ky-saturn",
	.ops = &dpu_saturn_ops,
};

#ifndef MODULE
static int __init dpu_core_register(void)
#else
int dpu_core_register(void)
#endif
{
	return dpu_core_ops_register(&entry);
}
#ifndef MODULE
subsys_initcall(dpu_core_register);
#endif

MODULE_LICENSE("GPL v2");

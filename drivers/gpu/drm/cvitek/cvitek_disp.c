#include <linux/of_platform.h>
#include <linux/component.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_vblank.h>
#include <drm/drm_fourcc.h>

#include "cvitek_vo_sys_reg.h"
#include "cvitek_drm.h"
#include "cvitek_disp.h"

#define DEFINE_CSC_COEF0(a, b, c) \
		.coef[0][0] = a, .coef[0][1] = b, .coef[0][2] = c,
#define DEFINE_CSC_COEF1(a, b, c) \
		.coef[1][0] = a, .coef[1][1] = b, .coef[1][2] = c,
#define DEFINE_CSC_COEF2(a, b, c) \
		.coef[2][0] = a, .coef[2][1] = b, .coef[2][2] = c,

#define RETRAIN_REG        0x281000F4
#define GP_REG             0x281000F8
#define DISP_FPS_CNT       10
#define DISP_FPS_TABLE_CNT 6
#define DISP_MAX_INST      2

static void *gp_reg;
static void *retrain_reg;

static struct disp_csc_matrix csc_mtrx[DISP_CSC_MAX] = {
	// none
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		0)
		DEFINE_CSC_COEF1(0,		BIT(10),	0)
		DEFINE_CSC_COEF2(0,		0,		BIT(10))
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// yuv2rgb
	// 601 Limited
	//  R = Y + 1.402* Pr                           //
	//  G = Y - 0.344 * Pb  - 0.792* Pr             //
	//  B = Y + 1.772 * Pb                          //
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		1436)
		DEFINE_CSC_COEF1(BIT(10),	BIT(13) | 352,	BIT(13) | 731)
		DEFINE_CSC_COEF2(BIT(10),	1815,		0)
		.sub[0] = 0,   .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// 601 Full
	//  R = 1.164 *(Y - 16) + 1.596 *(Cr - 128)                     //
	//  G = 1.164 *(Y - 16) - 0.392 *(Cb - 128) - 0.812 *(Cr - 128) //
	//  B = 1.164 *(Y - 16) + 2.016 *(Cb - 128)                     //
	{
		DEFINE_CSC_COEF0(1192,	0,		1634)
		DEFINE_CSC_COEF1(1192,	BIT(13) | 401,	BIT(13) | 833)
		DEFINE_CSC_COEF2(1192,	2065,		0)
		.sub[0] = 16,  .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// 709 Limited
	// R = Y + 1.540(Cr – 128)
	// G = Y - 0.183(Cb – 128) – 0.459(Cr – 128)
	// B = Y + 1.816(Cb – 128)
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		1577)
		DEFINE_CSC_COEF1(BIT(10),	BIT(13) | 187,	BIT(13) | 470)
		DEFINE_CSC_COEF2(BIT(10),	1860,		0)
		.sub[0] = 0,   .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// 709 Full
	//  R = 1.164 *(Y - 16) + 1.792 *(Cr - 128)                     //
	//  G = 1.164 *(Y - 16) - 0.213 *(Cb - 128) - 0.534 *(Cr - 128) //
	//  B = 1.164 *(Y - 16) + 2.114 *(Cb - 128)                     //
	{
		DEFINE_CSC_COEF0(1192,	0,		1836)
		DEFINE_CSC_COEF1(1192,	BIT(13) | 218,	BIT(13) | 547)
		DEFINE_CSC_COEF2(1192,	2166,		0)
		.sub[0] = 16,  .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// rgb2yuv
	// 601 Limited
	//  Y = 0.299 * R + 0.587 * G + 0.114 * B       //
	// Pb =-0.169 * R - 0.331 * G + 0.500 * B       //
	// Pr = 0.500 * R - 0.419 * G - 0.081 * B       //
	{
		DEFINE_CSC_COEF0(306,		601,		117)
		DEFINE_CSC_COEF1(BIT(13)|173,	BIT(13)|339,	512)
		DEFINE_CSC_COEF2(512,		BIT(13)|429,	BIT(13)|83)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 128, .add[2] = 128
	},
	// 601 Full
	//  Y = 16  + 0.257 * R + 0.504 * g + 0.098 * b //
	// Cb = 128 - 0.148 * R - 0.291 * g + 0.439 * b //
	// Cr = 128 + 0.439 * R - 0.368 * g - 0.071 * b //
	{
		DEFINE_CSC_COEF0(263,		516,		100)
		DEFINE_CSC_COEF1(BIT(13)|152,	BIT(13)|298,	450)
		DEFINE_CSC_COEF2(450,		BIT(13)|377,	BIT(13)|73)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 16,  .add[1] = 128, .add[2] = 128
	},
	// 709 Limited
	//   Y =       0.2126   0.7152   0.0722
	//  Cb = 128 - 0.1146  -0.3854   0.5000
	//  Cr = 128 + 0.5000  -0.4542  -0.0468
	{
		DEFINE_CSC_COEF0(218,		732,		74)
		DEFINE_CSC_COEF1(BIT(13)|117,	BIT(13)|395,	512)
		DEFINE_CSC_COEF2(512,		BIT(13)|465,	BIT(13)|48)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 128, .add[2] = 128
	},
	// 709 Full
	//  Y = 16  + 0.183 * R + 0.614 * g + 0.062 * b //
	// Cb = 128 - 0.101 * R - 0.339 * g + 0.439 * b //
	// Cr = 128 + 0.439 * R - 0.399 * g - 0.040 * b //
	{
		DEFINE_CSC_COEF0(187,		629,		63)
		DEFINE_CSC_COEF1(BIT(13)|103,	BIT(13)|347,	450)
		DEFINE_CSC_COEF2(450,		BIT(13)|408,	BIT(13)|41)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 16,  .add[1] = 128, .add[2] = 128
	},
};

static const struct disp_pattern patterns[DISP_PAT_MAX] = {
	{.type = DISP_PAT_TYPE_OFF,	.color = DISP_PAT_COLOR_MAX},
	{.type = DISP_PAT_TYPE_SNOW, .color = DISP_PAT_COLOR_MAX},
	{.type = DISP_PAT_TYPE_AUTO, .color = DISP_PAT_COLOR_MAX},
	{.type = DISP_PAT_TYPE_FULL, .color = DISP_PAT_COLOR_RED},
	{.type = DISP_PAT_TYPE_FULL, .color = DISP_PAT_COLOR_GREEN},
	{.type = DISP_PAT_TYPE_FULL, .color = DISP_PAT_COLOR_BLUE},
	{.type = DISP_PAT_TYPE_FULL, .color = DISP_PAT_COLOR_BAR},
	{.type = DISP_PAT_TYPE_H_GRAD, .color = DISP_PAT_COLOR_WHITE},
	{.type = DISP_PAT_TYPE_V_GRAD, .color = DISP_PAT_COLOR_WHITE},
	{.type = DISP_PAT_TYPE_FULL, .color = DISP_PAT_COLOR_USR,
	.rgb = {0, 0, 0} },
};

static void disp_reg_shadow_sel(u8 disp_id, bool read_shadow)
{
	_reg_write_mask(REG_DISP_CFG(disp_id), BIT(18),
			(read_shadow ? 0x0 : BIT(18)));
}

static void disp_reg_set_shadow_mask(struct disp_hw_ctx *ctx, bool shadow_mask)
{
	if (shadow_mask)
		spin_lock(&ctx->disp_mask_spinlock);

	_reg_write_mask(REG_DISP_CFG(ctx->disp_id), BIT(17),
			(shadow_mask ? BIT(17) : 0x0));

	if (!shadow_mask)
		spin_unlock(&ctx->disp_mask_spinlock);
}

void disp_set_window_bgcolor(u8 disp_id, u16 r, u16 g, u16 b)
{
	_reg_write(REG_DISP_PAT_COLOR3(disp_id), g << 16 | r);
	_reg_write_mask(REG_DISP_PAT_COLOR4(disp_id), 0x0fff, b);
}

void disp_enable_window_bgcolor(u8 disp_id, bool enable)
{
	_reg_write_mask(REG_DISP_PAT_CFG(disp_id), 0x20, enable ? 0x20 : 0);
}

static bool disp_tgen_enable(u8 disp_id, bool enable)
{
	bool is_enable = (_reg_read(REG_DISP_CFG(disp_id)) & 0x80);

	if (is_enable != enable) {
		_reg_write_mask(REG_DISP_CFG(disp_id), 0x0080,
				enable ? 0x80 : 0x00);
	}

	is_enable = (_reg_read(REG_DISP_CFG(disp_id)) & 0x80);

	return is_enable;
}

static void disp_set_addr(struct disp_hw_ctx *ctx, u64 addr0, u64 addr1, u64 addr2)
{
	disp_reg_set_shadow_mask(ctx, true);

	_reg_write(REG_DISP_ADDR0_L(ctx->disp_id), addr0);
	_reg_write(REG_DISP_ADDR0_H(ctx->disp_id), addr0 >> 32);
	_reg_write(REG_DISP_ADDR1_L(ctx->disp_id), addr1);
	_reg_write(REG_DISP_ADDR1_H(ctx->disp_id), addr1 >> 32);
	_reg_write(REG_DISP_ADDR2_L(ctx->disp_id), addr2);
	_reg_write(REG_DISP_ADDR2_H(ctx->disp_id), addr2 >> 32);

	disp_reg_set_shadow_mask(ctx, false);
}

static void disp_set_csc(u8 disp_id, struct disp_csc_matrix *cfg)
{
	_reg_write(REG_DISP_IN_CSC0(disp_id), BIT(31) |
		   (cfg->coef[0][1] << 16) | (cfg->coef[0][0]));
	_reg_write(REG_DISP_IN_CSC1(disp_id),
		   (cfg->coef[1][0] << 16) | (cfg->coef[0][2]));
	_reg_write(REG_DISP_IN_CSC2(disp_id),
		   (cfg->coef[1][2] << 16) | (cfg->coef[1][1]));
	_reg_write(REG_DISP_IN_CSC3(disp_id),
		   (cfg->coef[2][1] << 16) | (cfg->coef[2][0]));
	_reg_write(REG_DISP_IN_CSC4(disp_id), (cfg->coef[2][2]));
	_reg_write(REG_DISP_IN_CSC_SUB(disp_id),
		   (cfg->sub[2] << 16) | (cfg->sub[1] << 8) |
		   cfg->sub[0]);
	_reg_write(REG_DISP_IN_CSC_ADD(disp_id),
		   (cfg->add[2] << 16) | (cfg->add[1] << 8) |
		   cfg->add[0]);
}

static void disp_set_in_csc(struct disp_hw_ctx *ctx, enum disp_csc csc)
{
	if (csc == DISP_CSC_NONE)
		_reg_write(REG_DISP_IN_CSC0(ctx->disp_id), 0);
	else if (csc < DISP_CSC_MAX)
		disp_set_csc(ctx->disp_id, &csc_mtrx[csc]);

	ctx->disp_cfg.in_csc = csc;
}

#if 0
static void disp_set_out_csc(struct disp_hw_ctx *ctx, enum disp_csc csc)
{
	if (csc == DISP_CSC_NONE) {
		_reg_write(REG_DISP_OUT_CSC0(ctx->disp_id), 0);
	} else if (csc < DISP_CSC_MAX) {
		struct disp_csc_matrix *cfg = &csc_mtrx[csc];

		_reg_write(REG_DISP_OUT_CSC0(ctx->disp_id), BIT(31) |
			   (cfg->coef[0][1] << 16) | (cfg->coef[0][0]));
		_reg_write(REG_DISP_OUT_CSC1(ctx->disp_id),
			   (cfg->coef[1][0] << 16) | (cfg->coef[0][2]));
		_reg_write(REG_DISP_OUT_CSC2(ctx->disp_id),
			   (cfg->coef[1][2] << 16) | (cfg->coef[1][1]));
		_reg_write(REG_DISP_OUT_CSC3(ctx->disp_id),
			   (cfg->coef[2][1] << 16) | (cfg->coef[2][0]));
		_reg_write(REG_DISP_OUT_CSC4(ctx->disp_id), (cfg->coef[2][2]));
		_reg_write(REG_DISP_OUT_CSC_SUB(ctx->disp_id),
			   (cfg->sub[2] << 16) | (cfg->sub[1] << 8) |
			   cfg->sub[0]);
		_reg_write(REG_DISP_OUT_CSC_ADD(ctx->disp_id),
			   (cfg->add[2] << 16) | (cfg->add[1] << 8) |
			   cfg->add[0]);
	}

	ctx->disp_cfg.out_csc = csc;
}

static void disp_set_pattern(u8 disp_id, enum disp_pat_type type,
			   enum disp_pat_color color, const u16 *rgb)
{
	switch (type) {
	case DISP_PAT_TYPE_OFF:
		_reg_write_mask(REG_DISP_PAT_CFG(disp_id), 0x16, 0);
		break;

	case DISP_PAT_TYPE_SNOW:
		_reg_write_mask(REG_DISP_PAT_CFG(disp_id), 0x16, 0x10);
		break;

	case DISP_PAT_TYPE_AUTO:
		_reg_write(REG_DISP_PAT_COLOR0(disp_id), 0x03ff03ff);
		_reg_write_mask(REG_DISP_PAT_COLOR1(disp_id), 0x000003ff, 0x3ff);
		_reg_write_mask(REG_DISP_PAT_CFG(disp_id), 0xff0016,
				0x780006);
		break;

	case DISP_PAT_TYPE_V_GRAD:
	case DISP_PAT_TYPE_H_GRAD:
	case DISP_PAT_TYPE_FULL: {
		if (color == DISP_PAT_COLOR_USR) {
			_reg_write(REG_DISP_PAT_COLOR0(disp_id), rgb[1] << 16 | rgb[0]);
			_reg_write_mask(REG_DISP_PAT_COLOR1(disp_id), 0x000003ff, rgb[2]);
			_reg_write_mask(REG_DISP_PAT_CFG(disp_id), 0x1f000016,
					(type << 27) | (DISP_PAT_COLOR_WHITE << 24) | 0x0002);
		} else {
			_reg_write(REG_DISP_PAT_COLOR0(disp_id), 0x03ff03ff);
			_reg_write_mask(REG_DISP_PAT_COLOR1(disp_id), 0x000003ff, 0x3ff);
			_reg_write_mask(REG_DISP_PAT_CFG(disp_id), 0x1f000016,
					(type << 27) | (color << 24) | 0x0002);
		}
		break;
	}
	default:
		DRM_ERROR("%s - unacceptiable pattern-type(%d)\n", __func__, type);
		break;
	}
}
#endif

static void drm_timing_to_cvi_timing(u8 disp_id, struct disp_timing *timing, struct drm_display_mode *mode)
{
	u32 hfp, hbp, hsw, vfp, vbp, vsw;

	timing->width = mode->hdisplay;
	timing->height = mode->vdisplay;

	hfp = mode->hsync_start - mode->hdisplay;
	hbp = mode->htotal - mode->hsync_end;
	hsw = mode->hsync_end - mode->hsync_start;
	vfp = mode->vsync_start - mode->vdisplay;
	vbp = mode->vtotal - mode->vsync_end;
	vsw = mode->vsync_end - mode->vsync_start;

	//cvitek regs
	timing->vtotal = mode->vtotal - 1;
	timing->htotal = mode->htotal - 1;
	timing->vsync_start = 0;
	timing->vsync_end = timing->vsync_start + vsw - 1;
	timing->vfde_start = timing->vmde_start = timing->vsync_start + vsw + vbp;
	timing->vfde_end = timing->vmde_end = timing->vfde_start + timing->height - 1;
	timing->hsync_start = 0;
	timing->hsync_end = timing->hsync_start + hsw - 1;
	timing->hfde_start = timing->hmde_start = timing->hsync_start + hsw + hbp;
	timing->hfde_end = timing->hmde_end = timing->hfde_start + timing->width - 1;

}

static void disp_set_mode_timing(struct disp_hw_ctx *ctx,
			     struct drm_display_mode *adj_mode)
{
	u8 disp_id = ctx->disp_id;
	u32 tmp = 0;

	drm_timing_to_cvi_timing(ctx->disp_id, &ctx->disp_timing, adj_mode);

	disp_tgen_enable(disp_id, false);

	if (adj_mode->flags & DRM_MODE_FLAG_NVSYNC)
		tmp |= 0x20;
	if (adj_mode->flags & DRM_MODE_FLAG_NHSYNC)
		tmp |= 0x40;

	_reg_write_mask(REG_DISP_CFG(disp_id), 0x0060, tmp);
	_reg_write(REG_DISP_TOTAL(disp_id),
		   (ctx->disp_timing.htotal << 16) | ctx->disp_timing.vtotal);
	_reg_write(REG_DISP_VSYNC(disp_id),
		   (ctx->disp_timing.vsync_end << 16) | ctx->disp_timing.vsync_start);
	_reg_write(REG_DISP_VFDE(disp_id),
		   (ctx->disp_timing.vfde_end << 16) | ctx->disp_timing.vfde_start);
	_reg_write(REG_DISP_VMDE(disp_id),
		   (ctx->disp_timing.vmde_end << 16) | ctx->disp_timing.vmde_start);
	_reg_write(REG_DISP_HSYNC(disp_id),
		   (ctx->disp_timing.hsync_end << 16) | ctx->disp_timing.hsync_start);
	_reg_write(REG_DISP_HFDE(disp_id),
		   (ctx->disp_timing.hfde_end << 16) | ctx->disp_timing.hfde_start);
	_reg_write(REG_DISP_HMDE(disp_id),
		   (ctx->disp_timing.hmde_end << 16) | ctx->disp_timing.hmde_start);

	// force update
	_reg_write_mask(REG_DISP_CFG(ctx->disp_id), 0x10000, 1 << 16);
	_reg_write_mask(REG_DISP_CFG(ctx->disp_id), 0x10000, 1 << 16);

	disp_tgen_enable(disp_id, true);
}

/* convert from fourcc format to disp format */
static u32 disp_get_format(u32 pixel_format)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(disp_formats); i++)
		if (disp_formats[i].pixel_format == pixel_format)
			return disp_formats[i].hw_format;

	/* not found */
	DRM_ERROR("Not found pixel format!!fourcc_format= %d\n",
		  pixel_format);
	return DISP_FORMAT_UNSUPPORT;
}

/* convert from fourcc format to vgop format */
static u32 vgop_get_format(u32 pixel_format)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(vgop_formats); i++)
		if (vgop_formats[i].pixel_format == pixel_format)
			return vgop_formats[i].hw_format;

	/* not found */
	DRM_ERROR("Not found pixel format!!fourcc_format= %d\n",
		  pixel_format);
	return VGOP_FORMAT_UNSUPPORT;
}

static void disp_set_framedone_interrupt(u8 disp_id, bool enable)
{
	// enable or disable frame done intr
	_reg_write(REG_DISP_INTER_SEL(disp_id), enable ? 0x2 : 0x0);
	// frame done mask
	_reg_write_mask(REG_DISP_INTER_CLR(disp_id), 0xff00, 0x3 << 8);
}

static union disp_dbg_status disp_dbg_status(u8 disp_id)
{
	union disp_dbg_status status;

	status.raw = _reg_read(REG_DISP_DBG(disp_id));

	status.b.err_fwr_clr = 1;
	status.b.err_erd_clr = 1;
	status.b.bw_fail_clr = 1;
	status.b.osd_bw_fail_clr = 1;
	_reg_write(REG_DISP_DBG(disp_id), status.raw);

	return status;
}

static union disp_intr disp_intr_status(u8 disp_id)
{
	union disp_intr status;

	status.raw = _reg_read(REG_DISP_DEBUG_STATUS(disp_id));
	return status;
}

static void disp_intr_clr(u8 disp_id, union disp_intr intr_clr)
{
	u32 tmp = 0;

	if (intr_clr.b.disp_frame_done)
		tmp |= BIT(0);
	if (intr_clr.b.up_1t)
		tmp |= BIT(1);
	if (intr_clr.b.up_1t_lite)
		tmp |= BIT(2);

	_reg_write_mask(REG_DISP_INTER_CLR(disp_id), 0x7, tmp);

	if (intr_clr.b.fifo_full_error)
		_reg_write_mask(REG_DISP_ODMA_FIFO_CFG(disp_id), BIT(9), BIT(9));
}

static void disp_crtc_finish_page_flip(struct cvitek_crtc *crtc)
{
	struct drm_device *dev = crtc->base.dev;
	unsigned long flags;

	spin_lock_irqsave(&dev->event_lock, flags);
	if (crtc->event) {
		drm_crtc_send_vblank_event(&crtc->base, crtc->event);
		drm_crtc_vblank_put(&crtc->base);
		crtc->event = NULL;
	}
	spin_unlock_irqrestore(&dev->event_lock, flags);
}

u64 timespec64_to_us(struct timespec64 *disp_time_spec)
{
	return disp_time_spec->tv_sec * 1000000L + disp_time_spec->tv_nsec / 1000L;
}

bool ddr_need_retrain(void)
{
	if (retrain_reg)
		return ioread32(retrain_reg) & 0xF;
	else
		return false;
}

void trigger_8051(void)
{
	if (gp_reg && retrain_reg) {
		iowrite32(ioread32(gp_reg) | 0x10000, gp_reg);
		iowrite32(ioread32(retrain_reg) & ~0xF, retrain_reg);
	}
}

bool disp_check_tgen_enable(u8 inst)
{
	bool is_enable = (_reg_read(REG_DISP_CFG(inst)) & 0x80);

	return is_enable;
}

static void ddr_retrain(int disp_id, union disp_intr intr_status)
{

	static struct timespec64 disp_last_overlap_time;
	static const u8 disp_fps_table[DISP_FPS_TABLE_CNT] = {24, 25, 30, 48, 50, 60};
	static const u16 disp_margin_table[DISP_FPS_TABLE_CNT] = {489, 460, 345, 129, 115, 57};
	static u8 disp_fps[DISP_MAX_INST], disp_fps_cnt[DISP_MAX_INST];
	static u16 disp_margin_time[DISP_MAX_INST];
	static struct timespec64 disp_frame_end_time[DISP_MAX_INST];
	static struct timespec64 disp_fps_time[DISP_MAX_INST];
	u64 disp_frame_end_time_us[DISP_MAX_INST], disp_frame_gap_time_ms[DISP_MAX_INST];
	struct timespec64 disp_cur_overlap_time;
	u8 i = 0;

	if (intr_status.b.disp_frame_done) {
		disp_cur_overlap_time = disp_frame_end_time[disp_id] = ktime_to_timespec64(ktime_get());
		disp_frame_end_time_us[disp_id] = timespec64_to_us(&disp_frame_end_time[disp_id]);
		disp_frame_end_time_us[!disp_id] = timespec64_to_us(&disp_frame_end_time[!disp_id]);

		if (disp_fps_cnt[disp_id] < DISP_FPS_CNT) {
			if (!disp_fps_cnt[disp_id])
				disp_fps_time[disp_id] = disp_frame_end_time[disp_id];
			disp_fps_cnt[disp_id]++;
		} else if (disp_fps_cnt[disp_id] == DISP_FPS_CNT) {
			disp_fps_cnt[disp_id]++;
			disp_frame_gap_time_ms[disp_id] = disp_frame_end_time_us[disp_id] / 1000L
							- timespec64_to_us(&disp_fps_time[disp_id]) / 1000L;
			disp_fps[disp_id] = (DISP_FPS_CNT * 10000 / disp_frame_gap_time_ms[disp_id] + 5) / 10;
			DRM_DEBUG("disp%d fps:%d\n", disp_id, disp_fps[disp_id]);
		}

		if (disp_frame_end_time_us[disp_id] > disp_frame_end_time_us[!disp_id]) {
			if ((disp_margin_time[disp_id] == 0) && (disp_fps[!disp_id] != 0)) {
				for (i = 0; i < DISP_FPS_TABLE_CNT; i++) {
					if ((disp_fps[!disp_id] <= disp_fps_table[i])
						|| (i == DISP_FPS_TABLE_CNT - 1)) {
						disp_margin_time[disp_id] = disp_margin_table[i];
						DRM_DEBUG("disp%d ahead disp%d, margin time(%d)\n"
							, !disp_id, disp_id, disp_margin_time[disp_id]);
						break;
					}
				}
			}
			if ((disp_frame_end_time_us[disp_id] - disp_frame_end_time_us[!disp_id]
				< disp_margin_time[disp_id]) || !disp_check_tgen_enable(!disp_id)) {
				if (ddr_need_retrain())
					trigger_8051();
				if (timespec64_to_us(&disp_cur_overlap_time)
					- timespec64_to_us(&disp_last_overlap_time) > 900000L)
					DRM_DEBUG("disp%d ahead disp%d, vblanking overlap. "
						"margin(%lld)us last overlap is (%lld)ms before.\n"
						, !disp_id, disp_id
						, disp_frame_end_time_us[disp_id] - disp_frame_end_time_us[!disp_id]
						, (timespec64_to_us(&disp_cur_overlap_time)
							- timespec64_to_us(&disp_last_overlap_time)) / 1000);
				disp_last_overlap_time = disp_cur_overlap_time;
			}
		}
	}
}


static irqreturn_t disp_irq_handler(int irq, void *data)
{
	struct device *disp_dev = data;
	struct cvitek_disp *cvitek_disp = dev_get_drvdata(disp_dev);
	struct disp_hw_ctx *ctx = cvitek_disp->hw_ctx;
	struct drm_crtc *crtc = &cvitek_disp->crtc.base;

	union disp_dbg_status dbg_status = disp_dbg_status(ctx->disp_id);
	union disp_intr intr_status = disp_intr_status(ctx->disp_id);

	if (dbg_status.b.bw_fail) {
		if (!ctx->primary_formats_xr24)
			DRM_DEBUG_DRIVER(" disp bw failed !!!\n");
	}

	if (dbg_status.b.osd_bw_fail)
		DRM_DEBUG_DRIVER(" osd bw failed !!!\n");

	/* IC bug need clear twice */
	disp_intr_clr(ctx->disp_id, intr_status);
	disp_intr_clr(ctx->disp_id, intr_status);

	if (intr_status.b.disp_frame_done)
		ddr_retrain(ctx->disp_id, intr_status);

	/* vblank irq */
	if (intr_status.b.up_1t) {
		drm_crtc_handle_vblank(crtc);
		disp_crtc_finish_page_flip(to_cvitek_crtc(crtc));
	}

	return IRQ_HANDLED;
}

static int disp_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct cvitek_crtc *ccrtc = to_cvitek_crtc(crtc);
	struct disp_hw_ctx *ctx = ccrtc->hw_ctx;

	DRM_DEBUG_DRIVER("---- enter disp crtc enable vblank. ----\n");

	disp_set_framedone_interrupt(ctx->disp_id, true);

	return 0;
}

static void disp_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct cvitek_crtc *ccrtc = to_cvitek_crtc(crtc);
	struct disp_hw_ctx *ctx = ccrtc->hw_ctx;

	DRM_DEBUG_DRIVER("---- enter disp crtc disable vblank. ----\n");

	disp_set_framedone_interrupt(ctx->disp_id, false);
}

static void disp_crtc_write_gamma_lut(struct cvitek_crtc *ccrtc, struct drm_crtc *crtc,
			       struct drm_crtc_state *old_state)
{
	struct disp_hw_ctx *ctx = ccrtc->hw_ctx;
	struct drm_color_lut *lut = NULL;
	uint32_t i, r, g, b, word;

	_reg_write_mask(REG_DISP_GAMMA_CTRL(ctx->disp_id), 0x03, 0x03);

	if (!crtc->state->gamma_lut) {
		_reg_write_mask(REG_DISP_GAMMA_CTRL(ctx->disp_id), 0x03, 0x00);
		return;
	}

	lut = (struct drm_color_lut *)crtc->state->gamma_lut->data;

	for (i = 0; i < crtc->gamma_size; ++i) {
		r = drm_color_lut_extract(lut[i].red, 8);
		g = drm_color_lut_extract(lut[i].green, 8);
		b = drm_color_lut_extract(lut[i].blue, 8);
		word = r | (g << 8) | (b << 16)
			| (i << 24) | 0x80000000;
		_reg_write(REG_DISP_GAMMA_WR_LUT(ctx->disp_id), word);
	}

	_reg_write_mask(REG_DISP_GAMMA_CTRL(ctx->disp_id), 0x03, 0x00);
	_reg_write_mask(REG_DISP_GAMMA_CTRL(ctx->disp_id), 0x0C, 0x04);

}

static void disp_crtc_atomic_enable(struct drm_crtc *crtc,
				   struct drm_crtc_state *old_state)
{
	struct cvitek_crtc *ccrtc = to_cvitek_crtc(crtc);
	struct disp_hw_ctx *ctx = ccrtc->hw_ctx;
	bool is_enable;

	DRM_DEBUG_DRIVER("---- enter disp crtc atomic enable. ----\n");

	disp_enable_window_bgcolor(ctx->disp_id, false);

	is_enable = disp_tgen_enable(ctx->disp_id, true);
	while (is_enable != true)
		is_enable = disp_tgen_enable(ctx->disp_id, true);

	// if the state have a GAMMA LUT, need to update
	if (crtc->state->gamma_lut)
		disp_crtc_write_gamma_lut(ccrtc, crtc, old_state);

	drm_crtc_vblank_on(crtc);
}

static void disp_crtc_atomic_disable(struct drm_crtc *crtc,
				    struct drm_crtc_state *old_state)
{
	struct cvitek_crtc *ccrtc = to_cvitek_crtc(crtc);
	struct drm_device *drm = crtc->dev;
	struct disp_hw_ctx *ctx = ccrtc->hw_ctx;
	bool is_enable;

	DRM_DEBUG_DRIVER("---- enter disp crtc atomic disable. ----\n");

	drm_crtc_vblank_off(crtc);

	is_enable = disp_tgen_enable(ctx->disp_id, false);
	while (is_enable != false)
		is_enable = disp_tgen_enable(ctx->disp_id, false);

	disp_set_window_bgcolor(ctx->disp_id, 0, 0, 0);
	disp_enable_window_bgcolor(ctx->disp_id, true);

	spin_lock_irq(&drm->event_lock);
	if (crtc->state->event) {
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		crtc->state->event = NULL;
	}
	spin_unlock_irq(&drm->event_lock);
}

static void disp_crtc_mode_set_nofb(struct drm_crtc *crtc)
{
	struct cvitek_crtc *ccrtc = to_cvitek_crtc(crtc);
	struct disp_hw_ctx *ctx = ccrtc->hw_ctx;
	struct drm_display_mode *adj_mode = &crtc->state->adjusted_mode;

	DRM_DEBUG_DRIVER("---- enter disp crtc mode set nofb. ----\n");

	disp_reg_shadow_sel(ctx->disp_id, false);
	disp_set_mode_timing(ctx, adj_mode);
}

static void disp_crtc_atomic_flush(struct drm_crtc *crtc,
				  struct drm_crtc_state *old_state)
{
	struct cvitek_crtc *ccrtc = to_cvitek_crtc(crtc);

	DRM_DEBUG_DRIVER("---- enter disp crtc atomic flush. ----\n");

	// force update
	// _reg_write_mask(REG_DISP_CFG(ctx->disp_id), 0x10000, 1 << 16);

	/*
	 * Only update GAMMA if the 'active' flag is not changed,
	 * otherwise it's updated by .atomic_enable.
	 */
	if (crtc->state->color_mgmt_changed &&
	    !crtc->state->active_changed) {
		disp_crtc_write_gamma_lut(ccrtc, crtc, old_state);
	}

	if (crtc->state->event) {
		crtc->state->event->pipe = drm_crtc_index(crtc);

		WARN_ON(drm_crtc_vblank_get(crtc) != 0);

		ccrtc->event = crtc->state->event;
		crtc->state->event = NULL;
	}
}

static const struct drm_crtc_helper_funcs disp_crtc_helper_funcs = {
	.mode_set_nofb	= disp_crtc_mode_set_nofb,
	.atomic_flush	= disp_crtc_atomic_flush,
	.atomic_enable	= disp_crtc_atomic_enable,
	.atomic_disable	= disp_crtc_atomic_disable,
};

static const struct drm_crtc_funcs disp_crtc_funcs = {
	.destroy	= drm_crtc_cleanup,
	.set_config	= drm_atomic_helper_set_config,
	.page_flip	= drm_atomic_helper_page_flip,
	.reset		= drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state	= drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_crtc_destroy_state,
	.enable_vblank	= disp_crtc_enable_vblank,
	.disable_vblank	= disp_crtc_disable_vblank,
	.gamma_set = drm_atomic_helper_legacy_gamma_set,
};

static int disp_set_rect(struct disp_hw_ctx *ctx, struct disp_rect rect)
{
	if ((rect.y > ctx->disp_timing.vfde_end) ||
	    (rect.x > ctx->disp_timing.hfde_end) ||
	    ((ctx->disp_timing.vfde_start + rect.y + rect.h - 1) >
	      ctx->disp_timing.vfde_end) ||
	    ((ctx->disp_timing.hfde_start + rect.x + rect.w - 1) >
	      ctx->disp_timing.hfde_end)) {
		DRM_ERROR("[drm][disp] %s: me's pos(%d, %d) size(%d, %d) ",
		       __func__, rect.x, rect.y, rect.w, rect.h);
		DRM_ERROR(" out of range(%d, %d).\n",
			ctx->disp_timing.hfde_end, ctx->disp_timing.vfde_end);
		return -EINVAL;
	}

	ctx->disp_timing.vmde_start = rect.y + ctx->disp_timing.vfde_start;
	ctx->disp_timing.hmde_start = rect.x + ctx->disp_timing.hfde_start;
	ctx->disp_timing.vmde_end = ctx->disp_timing.vmde_start + rect.h - 1;
	ctx->disp_timing.hmde_end = ctx->disp_timing.hmde_start + rect.w - 1;

	disp_reg_set_shadow_mask(ctx, true);

	_reg_write(REG_DISP_HMDE(ctx->disp_id),
		   (ctx->disp_timing.hmde_end << 16) | ctx->disp_timing.hmde_start);
	_reg_write(REG_DISP_VMDE(ctx->disp_id),
		   (ctx->disp_timing.vmde_end << 16) | ctx->disp_timing.vmde_start);

	disp_reg_set_shadow_mask(ctx, false);
	return 0;
}

static void disp_set_mem(struct disp_hw_ctx *ctx, struct disp_mem *mem)
{
	disp_reg_set_shadow_mask(ctx, true);

	_reg_write(REG_DISP_OFFSET(ctx->disp_id),
		   (mem->start_y << 16) | mem->start_x);
	_reg_write(REG_DISP_SIZE(ctx->disp_id),
		   ((mem->height - 1) << 16) | (mem->width - 1));
	_reg_write_mask(REG_DISP_PITCH_Y(ctx->disp_id), 0x00ffffff,
			mem->pitch_y);
	_reg_write(REG_DISP_PITCH_C(ctx->disp_id), mem->pitch_c);

	disp_reg_set_shadow_mask(ctx, false);

	disp_set_addr(ctx, mem->addr0, mem->addr1, mem->addr2);
}

void disp_gop_ow_set_cfg(struct disp_hw_ctx *ctx, u8 layer, u8 window, struct disp_gop_ow_cfg *ow_cfg)
{
	disp_reg_set_shadow_mask(ctx, true);

	_reg_write(REG_DISP_GOP_FMT(ctx->disp_id, layer, window),
			   ow_cfg->fmt);
	_reg_write(REG_DISP_GOP_H_RANGE(ctx->disp_id, layer, window),
			   (ow_cfg->end.x << 16) | ow_cfg->start.x);
	_reg_write(REG_DISP_GOP_V_RANGE(ctx->disp_id, layer, window),
			   (ow_cfg->end.y << 16) | ow_cfg->start.y);
	_reg_write(REG_DISP_GOP_ADDR_L(ctx->disp_id, layer, window),
			   ow_cfg->addr & 0xFFFFFFFF);
	_reg_write(REG_DISP_GOP_ADDR_H(ctx->disp_id, layer, window),
			   ow_cfg->addr >> 32);
	_reg_write(REG_DISP_GOP_CROP_PITCH(ctx->disp_id, layer, window),
			   (ow_cfg->crop_pixels << 16) | ow_cfg->pitch);
	_reg_write(REG_DISP_GOP_SIZE(ctx->disp_id, layer, window),
			   (ow_cfg->mem_size.h << 16) | ow_cfg->mem_size.w);

	disp_reg_set_shadow_mask(ctx, false);
}

void disp_gop_set_cfg(struct disp_hw_ctx *ctx, u8 layer, struct disp_gop_cfg *cfg)
{
	disp_reg_set_shadow_mask(ctx, true);

	_reg_write(REG_DISP_GOP_CFG(ctx->disp_id, layer), cfg->gop_ctrl.raw);

	disp_reg_set_shadow_mask(ctx, false);
}

static int disp_plane_atomic_check(struct drm_plane *plane,
				  struct drm_plane_state *state)
{
	struct drm_framebuffer *fb = state->fb;
	struct drm_crtc *crtc = state->crtc;
	struct drm_crtc_state *crtc_state;
	u32 src_x = state->src_x >> 16;
	u32 src_y = state->src_y >> 16;
	u32 src_w = state->src_w >> 16;
	u32 src_h = state->src_h >> 16;
	int crtc_x = state->crtc_x;
	int crtc_y = state->crtc_y;
	u32 crtc_w = state->crtc_w;
	u32 crtc_h = state->crtc_h;
	u32 fmt;

	DRM_DEBUG_DRIVER("---- enter disp plane atomic check. ----\n");

	if (!crtc || !fb)
		return 0;

	if (plane->type == DRM_PLANE_TYPE_PRIMARY) {
		fmt = disp_get_format(fb->format->format);
		if (fmt == DISP_FORMAT_UNSUPPORT) {
			DRM_ERROR("Display not support fmt\n");
			return -EINVAL;
		}

		if (src_w != crtc_w || src_h != crtc_h) {
			DRM_ERROR("Display not support scale, pelase use vpss\n");
			return -EINVAL;
		}

		if (src_x + src_w > fb->width ||
			src_y + src_h > fb->height) {
			DRM_ERROR("out of range : src_x + src_w > fb->width || src_y + src_h > fb->height\n");
			return -EINVAL;
		}

		crtc_state = drm_atomic_get_crtc_state(state->state, crtc);
		if (IS_ERR(crtc_state))
			return PTR_ERR(crtc_state);

		if (crtc_x + crtc_w > crtc_state->adjusted_mode.hdisplay ||
			crtc_y + crtc_h > crtc_state->adjusted_mode.vdisplay) {
			DRM_ERROR("out of range :\n");
			DRM_ERROR("\tcrtc_x(%d) + crtc_w(%d) > crtc_state->adjusted_mode.hdisplay(%d)\n",
						crtc_x, crtc_w, crtc_state->adjusted_mode.hdisplay);
			DRM_ERROR("\tcrtc_y(%d) + crtc_h(%d) > crtc_state->adjusted_mode.vdisplay(%d)\n",
						crtc_y, crtc_h, crtc_state->adjusted_mode.vdisplay);

			return -EINVAL;
		}
	} else {
		fmt = vgop_get_format(fb->format->format);
		if (fmt == VGOP_FORMAT_UNSUPPORT) {
			DRM_ERROR("VGOP not support fmt\n");
			return -EINVAL;
		}

		if (src_x != 0 || src_y != 0) {
			DRM_ERROR("Vgop not support crop.\n");
			return -EINVAL;
		}

		if (src_w != fb->width || src_h != fb->height) {
			DRM_ERROR("Vgop not support scale, pelase use vpss\n");
			return -EINVAL;
		}

		if (src_w != crtc_w || src_h != crtc_h) {
			DRM_ERROR("Vgop not support scale, pelase use vpss\n");
			return -EINVAL;
		}
	}

	return 0;
}

// To avoid bw fail
static void disp_set_bw_cfg(u32 fmt, unsigned int crtc_w, u32 disp_id)
{
	if (fmt == DISP_FORMAT_XRGB_8888) {
		_reg_write(REG_DISP_LINE_BUFFER(disp_id), 0x0);
		_reg_write(REG_DISP_RD_TH(disp_id), 0x0);
		_reg_write(REG_DISP_FIFO(disp_id), 0x0);
		_reg_write_mask(REG_DISP_PITCH_Y(disp_id), 0xff000000, 0x0 << 24);
	} else {
		if (fmt == DISP_FORMAT_RGB_888 || fmt == DISP_FORMAT_BGR_888) {
			_reg_write(REG_DISP_LINE_BUFFER(disp_id), 0x1);
			_reg_write(REG_DISP_RD_TH(disp_id), 0x1);
			_reg_write(REG_DISP_FIFO(disp_id), 0x78);
		} else {
			_reg_write(REG_DISP_LINE_BUFFER(disp_id), 0x0);
			_reg_write(REG_DISP_RD_TH(disp_id), 0x0);

			if (fmt == DISP_FORMAT_YUV_PLANAR_420 || fmt == DISP_FORMAT_YUV_PLANAR_422)
				_reg_write(REG_DISP_FIFO(disp_id), 0x4400480);
			else
				_reg_write(REG_DISP_FIFO(disp_id), 0x4800480);
		}

		_reg_write_mask(REG_DISP_PITCH_Y(disp_id), 0xff000000, 0xff << 24);
	}
}

static void disp_update_channel(struct cvitek_plane *cplane,
				   enum drm_plane_type type,
			       struct drm_framebuffer *fb, int crtc_x,
			       int crtc_y, unsigned int crtc_w,
			       unsigned int crtc_h, u32 src_x,
			       u32 src_y, u32 src_w, u32 src_h)
{
	struct disp_hw_ctx *ctx = cplane->hw_ctx;
	u32 disp_id = ctx->disp_id;
	struct drm_gem_cma_object *cma_obj;
	u32 ch = cplane->ch;
	struct disp_rect rect;
	u64 addr;
	u32 fmt, i, bytesperpixel;
	enum drm_intf intf;

	if (type == DRM_PLANE_TYPE_PRIMARY) {
		//fmt
		fmt = disp_get_format(fb->format->format);

		if (fmt == DISP_FORMAT_XRGB_8888) {
			ctx->disp_vgop_status[0] = 1;
			_reg_write_mask(REG_DISP_CFG(disp_id), 0xf000,
							DISP_FORMAT_YUV_PLANAR_420 << 12);
			disp_set_in_csc(ctx, DISP_CSC_NONE);
			disp_set_bw_cfg(DISP_FORMAT_XRGB_8888, crtc_w, disp_id);

			rect.x = crtc_x;
			rect.y = crtc_y;
			rect.w = crtc_w;
			rect.h = crtc_h;

			disp_set_rect(ctx, rect);

			memset(&ctx->disp_cfg.mem, 0, sizeof(ctx->disp_cfg.mem));
			ctx->disp_cfg.mem.height = 1;
			ctx->disp_cfg.mem.width = 1;
			disp_set_mem(ctx, &ctx->disp_cfg.mem);

			//now only support ow_0
			ctx->disp_cfg.gop_cfg.gop_ctrl.b.ow0_en = 1;
			ctx->disp_cfg.gop_cfg.gop_ctrl.b.burst = 15;  // burst length set to 15

			cma_obj = drm_fb_cma_get_gem_obj(fb, 0);
			addr = cma_obj->paddr + fb->offsets[0];
			fmt = VGOP_FORMAT_ARGB8888;
			//ow_0
			ctx->disp_cfg.gop_cfg.ow_cfg[0].fmt = fmt;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].addr = addr;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].pitch = fb->pitches[0];
			ctx->disp_cfg.gop_cfg.ow_cfg[0].start.x = crtc_x;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].start.y = crtc_y;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.w = crtc_w;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.h = crtc_h;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].end.x = crtc_x + crtc_w;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].end.y = crtc_y + crtc_h;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].mem_size.w = ALIGN(ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.w
				* 4, 16);
			ctx->disp_cfg.gop_cfg.ow_cfg[0].mem_size.h = crtc_h;

			disp_gop_ow_set_cfg(ctx, 0, /*ow_0*/0, &ctx->disp_cfg.gop_cfg.ow_cfg[0]);

			disp_gop_set_cfg(ctx, 0, &ctx->disp_cfg.gop_cfg);
		} else {
			_reg_write_mask(REG_DISP_CFG(disp_id), 0xf000,
							fmt << 12);
			//csc
			switch (fmt) {
			case DISP_FORMAT_YUV_PLANAR_420:
			case DISP_FORMAT_YUV_PLANAR_422:
			case DISP_FORMAT_NV21:
			case DISP_FORMAT_NV12:
			case DISP_FORMAT_NV61:
			case DISP_FORMAT_NV16:
			case DISP_FORMAT_UYVY:
			case DISP_FORMAT_VYUY:
			case DISP_FORMAT_YUYV:
			case DISP_FORMAT_YVYU:
				disp_set_in_csc(ctx, DISP_CSC_601_LIMIT_YUV2RGB);
				break;
			case DISP_FORMAT_RGB_888:
			case DISP_FORMAT_BGR_888:
				disp_set_in_csc(ctx, DISP_CSC_NONE);
				break;
			default:
				DRM_DEBUG_DRIVER(" fmt:(%d) is not support\n", fmt);
				break;
			}

			disp_set_bw_cfg(fmt, crtc_w, disp_id);
			rect.x = crtc_x;
			rect.y = crtc_y;
			rect.w = crtc_w;
			rect.h = crtc_h;
			disp_set_rect(ctx, rect);

			for (i = 0; i < fb->format->num_planes; i++) {
				cma_obj = drm_fb_cma_get_gem_obj(fb, i);
				addr = cma_obj->paddr + fb->offsets[i];

				if (i == 0)
					ctx->disp_cfg.mem.addr0 = addr;
				else if (i == 1)
					ctx->disp_cfg.mem.addr1 = addr;
				else
					ctx->disp_cfg.mem.addr2 = addr;
			}

			ctx->disp_cfg.mem.pitch_y = fb->pitches[0];
			ctx->disp_cfg.mem.pitch_c = fb->pitches[1];
			ctx->disp_cfg.mem.start_x = src_x;
			ctx->disp_cfg.mem.start_y = src_y;
			ctx->disp_cfg.mem.width = src_w;
			ctx->disp_cfg.mem.height = src_h;
			disp_set_mem(ctx, &ctx->disp_cfg.mem);
		}
	} else {
		//now only support ow_0
		ctx->disp_vgop_status[ch - 1] = 1;

		ctx->disp_cfg.gop_cfg.gop_ctrl.b.ow0_en = 1;
		ctx->disp_cfg.gop_cfg.gop_ctrl.b.burst = 15;

		cma_obj = drm_fb_cma_get_gem_obj(fb, 0);
		addr = cma_obj->paddr + fb->offsets[0];
		fmt = vgop_get_format(fb->format->format);
		bytesperpixel = (fmt == VGOP_FORMAT_ARGB8888) ? 4 : 2;

		//ow_0
		ctx->disp_cfg.gop_cfg.ow_cfg[0].fmt = fmt;
		ctx->disp_cfg.gop_cfg.ow_cfg[0].addr = addr;
		ctx->disp_cfg.gop_cfg.ow_cfg[0].pitch = fb->pitches[0];

		if (crtc_x < 0) {
			ctx->disp_cfg.gop_cfg.ow_cfg[0].start.x = 0;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.w = crtc_w;
		} else if (crtc_x + crtc_w > ctx->disp_timing.width) {
			ctx->disp_cfg.gop_cfg.ow_cfg[0].start.x = crtc_x - 1;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.w = ctx->disp_timing.width - crtc_x + 1;
		} else {
			ctx->disp_cfg.gop_cfg.ow_cfg[0].start.x = crtc_x;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.w = crtc_w;
		}

		if (crtc_y < 0) {
			ctx->disp_cfg.gop_cfg.ow_cfg[0].start.y = 0;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].addr -= ctx->disp_cfg.gop_cfg.ow_cfg[0].pitch * crtc_y;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.h = crtc_h + crtc_y;
		} else if ((crtc_y + crtc_h) > ctx->disp_timing.height) {
			ctx->disp_cfg.gop_cfg.ow_cfg[0].start.y = crtc_y;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.h = ctx->disp_timing.height - crtc_y;
		} else {
			ctx->disp_cfg.gop_cfg.ow_cfg[0].start.y = crtc_y;
			ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.h = crtc_h;
		}

		ctx->disp_cfg.gop_cfg.ow_cfg[0].end.x = ctx->disp_cfg.gop_cfg.ow_cfg[0].start.x
				+ ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.w;
		ctx->disp_cfg.gop_cfg.ow_cfg[0].end.y = ctx->disp_cfg.gop_cfg.ow_cfg[0].start.y
				+ ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.h;
		ctx->disp_cfg.gop_cfg.ow_cfg[0].mem_size.w = ALIGN(ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.w
				* bytesperpixel, 16);

		ctx->disp_cfg.gop_cfg.ow_cfg[0].mem_size.h = crtc_h;

		disp_gop_ow_set_cfg(ctx, ch - 1, /*ow_0*/0, &ctx->disp_cfg.gop_cfg.ow_cfg[0]);

		disp_gop_set_cfg(ctx, ch - 1, &ctx->disp_cfg.gop_cfg);

		DRM_DEBUG_DRIVER("channel%d:imgsize(%d,%d) mem_size:(%d, %d)",
			ch, ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.w, ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.h,
			ctx->disp_cfg.gop_cfg.ow_cfg[0].mem_size.w, ctx->disp_cfg.gop_cfg.ow_cfg[0].mem_size.h);
		DRM_DEBUG_DRIVER("channel%d: vgop:(%d, %d)-%dx%d end(%d, %d) fmt:(%d)",
			ch, ctx->disp_cfg.gop_cfg.ow_cfg[0].start.x, ctx->disp_cfg.gop_cfg.ow_cfg[0].start.y,
			ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.w, ctx->disp_cfg.gop_cfg.ow_cfg[0].img_size.h,
			ctx->disp_cfg.gop_cfg.ow_cfg[0].end.x, ctx->disp_cfg.gop_cfg.ow_cfg[0].end.y, fmt);
	}

	intf = disp_id ? DRM_INTF_DISP1 : DRM_INTF_DISP0;
	extend_axi_to_36bit(addr >> 32, intf);

	DRM_DEBUG_DRIVER("channel%d: src:(%d, %d)-%dx%d, crtc:(%d, %d)-%dx%d fmt:(%d)",
			ch, src_x, src_y, src_w, src_h,
			crtc_x, crtc_y, crtc_w, crtc_h, fmt);
}

static void disp_plane_atomic_update(struct drm_plane *plane,
				    struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = plane->state;
	struct cvitek_plane *cplane = to_cvitek_plane(plane);
	struct disp_hw_ctx *ctx = cplane->hw_ctx;

	DRM_DEBUG_DRIVER("---- enter disp plane atomic update. ----\n");

	if (plane->type == DRM_PLANE_TYPE_PRIMARY) {
		if (disp_get_format(state->fb->format->format) == DISP_FORMAT_XRGB_8888)
			ctx->primary_formats_xr24 = true;
		else
			ctx->primary_formats_xr24 = false;
	}

	disp_update_channel(cplane, plane->type, state->fb, state->crtc_x, state->crtc_y,
			   state->crtc_w, state->crtc_h,
			   state->src_x >> 16, state->src_y >> 16,
			   state->src_w >> 16, state->src_h >> 16);

	disp_enable_window_bgcolor(ctx->disp_id, false);
}

static void disp_plane_atomic_disable(struct drm_plane *plane,
				     struct drm_plane_state *old_state)
{
	struct cvitek_plane *cplane = to_cvitek_plane(plane);
	struct disp_hw_ctx *ctx = cplane->hw_ctx;
	u32 ch = cplane->ch;
	int i = 0;

	DRM_DEBUG_DRIVER("---- enter disp plane atomic disable. ----\n");

	if (plane->type == DRM_PLANE_TYPE_PRIMARY) {
		disp_set_addr(ctx, 0, 0, 0);
		disp_set_window_bgcolor(ctx->disp_id, 0, 0, 0);
		disp_enable_window_bgcolor(ctx->disp_id, true);
		for (i = 0; i < CVITEK_MAX_PLANE - 1; i++) {
			if (ctx->disp_vgop_status[i] == 1) {
				memset(&ctx->disp_cfg.gop_cfg, 0, sizeof(struct disp_gop_cfg));
				disp_gop_set_cfg(ctx, i, &ctx->disp_cfg.gop_cfg);
				ctx->disp_vgop_status[i] = 0;
			}
		}
	} else {
		memset(&ctx->disp_cfg.gop_cfg, 0, sizeof(struct disp_gop_cfg));
		disp_gop_set_cfg(ctx, ch - 1, &ctx->disp_cfg.gop_cfg);
		ctx->disp_vgop_status[ch - 1] = 0;
	}
}

static const struct drm_plane_helper_funcs disp_plane_helper_funcs = {
	.atomic_check = disp_plane_atomic_check,
	.atomic_update = disp_plane_atomic_update,
	.atomic_disable = disp_plane_atomic_disable,
};

static struct drm_plane_funcs disp_plane_funcs = {
	.update_plane	= drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
};

static int cvitek_drm_crtc_init(struct platform_device *pdev, struct drm_device *dev, struct drm_crtc *crtc,
			       struct drm_plane *prim_plane, struct drm_plane *cursor_plane,
			       const struct disp_match_data *match_data)
{
	struct device_node *port;
	int ret;

	/* set crtc port so that
	 * drm_of_find_possible_crtcs call works
	 */
	port = of_graph_get_port_by_id(pdev->dev.of_node, 0);
	if (!port) {
		DRM_ERROR("no port node found in %pOF\n", pdev->dev.of_node);
		return -EINVAL;
	}
	of_node_put(port);
	crtc->port = port;

	ret = drm_crtc_init_with_planes(dev, crtc, prim_plane, cursor_plane,
					match_data->crtc_funcs, NULL);
	if (ret) {
		DRM_ERROR("failed to init crtc.\n");
		return ret;
	}

	drm_crtc_helper_add(crtc, match_data->crtc_helper_funcs);

	if (!gp_reg)
		gp_reg = ioremap(GP_REG, 4);
	if (!retrain_reg)
		retrain_reg = ioremap(RETRAIN_REG, 4);

	return 0;
}

static int cvitek_drm_plane_init(struct drm_device *dev, struct drm_plane *plane,
				enum drm_plane_type type,
				const struct disp_match_data *data)
{
	int ret = 0;

	ret = drm_universal_plane_init(dev, plane, 1 << dev->mode_config.num_crtc, data->plane_funcs,
				       (type == DRM_PLANE_TYPE_PRIMARY) ? data->primary_formats : data->overlay_formats,
				       (type == DRM_PLANE_TYPE_PRIMARY) ? data->primary_formats_cnt : data->overlay_formats_cnt,
				       NULL, type, NULL);
	if (ret) {
		DRM_ERROR("fail to init plane, ch=%d\n", 0);
		return ret;
	}

	drm_plane_helper_add(plane, data->plane_helper_funcs);

	return 0;
}

static void *disp_hw_ctx_alloc(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct disp_match_data *disp_data = (struct disp_match_data *)of_device_get_match_data(dev);
	struct disp_hw_ctx *ctx = NULL;
	int ret;
	int i = 0;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		DRM_ERROR("failed to alloc disp_hw_ctx\n");
		return ERR_PTR(-ENOMEM);
	}

	ctx->irq = platform_get_irq_byname(pdev, disp_data->irq_name);
	if (ctx->irq < 0) {
		DRM_ERROR("failed to get irq\n");
		return ERR_PTR(-ENODEV);
	}

	DRM_DEBUG_DRIVER("disp-interrupt: %d.\n", ctx->irq);

#if 0
	ctx->disp_clk = devm_clk_get(dev, "clk_disp");
	if (IS_ERR(ctx->disp_clk)) {
		DRM_ERROR("failed to parse clk disp\n");
		return ERR_PTR(-ENODEV);
	}
	DRM_DEBUG_DRIVER("clk_disp: %ldkhz.\n", clk_get_rate(ctx->disp_clk) / 1000);
#endif

	spin_lock_init(&ctx->disp_mask_spinlock);

	ctx->disp_id = disp_data->crtc_id;

	for (i = 0; i < CVITEK_MAX_PLANE; i++)
		ctx->disp_vgop_status[i] = 0;

	/* vblank irq init */
	ret = devm_request_irq(dev, ctx->irq, disp_irq_handler,
			       IRQF_SHARED, dev->driver->name, dev);
	if (ret)
		return ERR_PTR(-EIO);
	return ctx;
}

static void disp_hw_ctx_cleanup(struct platform_device *pdev, void *hw_ctx)
{
	struct disp_hw_ctx *ctx;

	if (hw_ctx) {
		ctx = hw_ctx;
		if (ctx->irq)
			disable_irq(ctx->irq);
		devm_kfree(&pdev->dev, hw_ctx);
	}
}

static int cvitek_disp_private_init(struct device *disp_dev, struct drm_device *drm_dev,
				  const struct disp_match_data *match_data)
{
	struct platform_device *pdev = to_platform_device(disp_dev);
	struct cvitek_disp *cvitek_disp;
	struct drm_plane *prim_plane;
	struct drm_plane *cursor_plane;
	enum drm_plane_type type;
	void *ctx = NULL;
	int ret;
	u32 ch;

	cvitek_disp = devm_kzalloc(disp_dev, sizeof(*cvitek_disp), GFP_KERNEL);
	if (!cvitek_disp) {
		DRM_ERROR("failed to alloc cvitek_disp\n");
		return -ENOMEM;
	}

	cvitek_disp->drm = drm_dev;
	cvitek_disp->hw_ctx = NULL;
	dev_set_drvdata(disp_dev, cvitek_disp);

	ctx = match_data->alloc_hw_ctx(pdev);
	if (IS_ERR(ctx)) {
		DRM_ERROR("failed to initialize cvitek_disp hw ctx\n");
		return -EINVAL;
	}

	cvitek_disp->hw_ctx = ctx;
	/*
	 * plane init
	 * TODO: Now only support primary plane, overlay planes
	 * need to do.
	 */
	for (ch = 0; ch < match_data->num_planes; ch++) {
		if (ch == match_data->prim_plane)
			type = DRM_PLANE_TYPE_PRIMARY;
		else if (ch == match_data->num_planes - 1)
			type = DRM_PLANE_TYPE_CURSOR;
		else
			type = DRM_PLANE_TYPE_OVERLAY;

		ret = cvitek_drm_plane_init(drm_dev, &cvitek_disp->planes[ch].base,
				type, match_data);
		if (ret)
			return ret;
		cvitek_disp->planes[ch].ch = ch;
		cvitek_disp->planes[ch].hw_ctx = ctx;
	}

	/* crtc init */
	prim_plane = &cvitek_disp->planes[match_data->prim_plane].base;
	cursor_plane = &cvitek_disp->planes[match_data->num_planes - 1].base;
	ret = cvitek_drm_crtc_init(pdev, drm_dev, &cvitek_disp->crtc.base,
				prim_plane, cursor_plane, match_data);
	if (ret)
		return ret;

	if (match_data->gamma_lut_size) {
		drm_mode_crtc_set_gamma_size(&cvitek_disp->crtc.base, match_data->gamma_lut_size);
		drm_crtc_enable_color_mgmt(&cvitek_disp->crtc.base, 0, 0, match_data->gamma_lut_size);
	}
	cvitek_disp->crtc.hw_ctx = ctx;

	return 0;
}

static void cvitek_disp_private_cleanup(struct device *disp_dev, struct drm_device *drm_dev)
{
	struct cvitek_disp *cvitek_disp = dev_get_drvdata(disp_dev);
	struct disp_match_data *disp_data;

	disp_data = (struct disp_match_data *)of_device_get_match_data(disp_dev);
	if (disp_data->cleanup_hw_ctx)
		disp_data->cleanup_hw_ctx(to_platform_device(disp_dev), cvitek_disp->hw_ctx);

	devm_kfree(disp_dev, cvitek_disp);
	disp_dev->driver_data = NULL;
}

static int cvitek_drm_kms_init(struct device *disp_dev, struct drm_device *drm_dev,
			      const struct disp_match_data *match_data)
{
	int ret;

	/* display controller init */
	ret = cvitek_disp_private_init(disp_dev, drm_dev, match_data);
	if (ret)
		goto err_mode_config_cleanup;

	return 0;

err_mode_config_cleanup:
	cvitek_disp_private_cleanup(disp_dev, drm_dev);
	return ret;
}

static int cvitek_drm_kms_cleanup(struct device *disp_dev, struct drm_device *drm_dev)
{
	cvitek_disp_private_cleanup(disp_dev, drm_dev);
	return 0;
}

static int cvitek_disp_bind(struct device *disp_dev, struct device *master, void *data)
{
	struct disp_match_data *disp_data;
	struct drm_device *drm_dev = data;
	int ret;

	DRM_DEBUG_DRIVER("---- enter cvitek disp bind. ----\n");

	disp_data = (struct disp_match_data *)of_device_get_match_data(disp_dev);
	if (!disp_data)
		return -EINVAL;

	/* display controller init */
	ret = cvitek_drm_kms_init(disp_dev, drm_dev, disp_data);
	if (ret)
		return ret;

	return 0;
}

static void cvitek_disp_unbind(struct device *disp_dev, struct device *master, void *data)
{
	struct drm_device *drm_dev = data;

	DRM_DEBUG_DRIVER("---- enter cvitek disp unbind. ----\n");

	cvitek_drm_kms_cleanup(disp_dev, drm_dev);
}

const struct component_ops cvitek_disp_ops = {
	.bind = cvitek_disp_bind,
	.unbind = cvitek_disp_unbind,
};
EXPORT_SYMBOL_GPL(cvitek_disp_ops);

struct disp_match_data cv186x_disp0 = {
	.crtc_id = 0,
	.num_planes = 4,
	.prim_plane = 0,
	.gamma_lut_size = 65,
	.irq_name = "disp0",
	.primary_formats = primary_formats,
	.primary_formats_cnt = ARRAY_SIZE(primary_formats),
	.overlay_formats = overlay_formats,
	.overlay_formats_cnt = ARRAY_SIZE(overlay_formats),
	.crtc_helper_funcs = &disp_crtc_helper_funcs,
	.crtc_funcs = &disp_crtc_funcs,
	.plane_helper_funcs = &disp_plane_helper_funcs,
	.plane_funcs = &disp_plane_funcs,

	.alloc_hw_ctx = disp_hw_ctx_alloc,
	.cleanup_hw_ctx = disp_hw_ctx_cleanup,
};

struct disp_match_data cv186x_disp1 = {
	.crtc_id = 1,
	.num_planes = 4,
	.prim_plane = 0,
	.gamma_lut_size = 65,
	.irq_name = "disp1",
	.primary_formats = primary_formats,
	.primary_formats_cnt = ARRAY_SIZE(primary_formats),
	.overlay_formats = overlay_formats,
	.overlay_formats_cnt = ARRAY_SIZE(overlay_formats),
	.crtc_helper_funcs = &disp_crtc_helper_funcs,
	.crtc_funcs = &disp_crtc_funcs,
	.plane_helper_funcs = &disp_plane_helper_funcs,
	.plane_funcs = &disp_plane_funcs,

	.alloc_hw_ctx = disp_hw_ctx_alloc,
	.cleanup_hw_ctx = disp_hw_ctx_cleanup,
};

static const struct of_device_id disp_match_table[] = {
	{ .compatible = "cvitek,cv186x_disp0",
	  .data = &cv186x_disp0 },
	{ .compatible = "cvitek,cv186x_disp1",
	  .data = &cv186x_disp1 },
	{ /* end node */ },
};
MODULE_DEVICE_TABLE(of, disp_match_table);

static int cvitek_disp_probe(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("---- enter cvitek disp probe. ----\n");

	return component_add(&pdev->dev, &cvitek_disp_ops);
}

static int cvitek_disp_remove(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("---- enter cvitek disp remove. ----\n");

	component_del(&pdev->dev, &cvitek_disp_ops);
	return 0;
}

struct platform_driver cvitek_disp_driver = {
	.probe = cvitek_disp_probe,
	.remove = cvitek_disp_remove,
	.driver = {
		.name = "cvitek-disp",
		.of_match_table = disp_match_table,
	},
};

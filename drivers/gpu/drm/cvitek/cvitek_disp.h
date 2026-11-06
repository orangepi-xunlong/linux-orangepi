#ifndef __CVITEK_DISP_H__
#define __CVITEK_DISP_H__

#define to_cvitek_crtc(crtc) \
	container_of(crtc, struct cvitek_crtc, base)

#define to_cvitek_plane(plane) \
	container_of(plane, struct cvitek_plane, base)

#define CVITEK_MAX_PLANE	4
#define DISP_MAX_GOP_OW_INST 8
#define DISP_MAX_GOP_FB_INST 2


/* cvitek-format translate table */
struct cvitek_format {
	u32 pixel_format;
	u32 hw_format;
};


enum DISP_FORMAT_E {
	DISP_FORMAT_YUV_PLANAR_420 = 0,
	DISP_FORMAT_YUV_PLANAR_422 = 1,

	DISP_FORMAT_RGB_888 = 4,
	DISP_FORMAT_BGR_888 = 3,

	DISP_FORMAT_NV21 = 9,
	DISP_FORMAT_NV12 = 8,
	DISP_FORMAT_NV16 = 10,
	DISP_FORMAT_NV61 = 11,

	DISP_FORMAT_UYVY = 15,
	DISP_FORMAT_VYUY = 14,
	DISP_FORMAT_YUYV = 13,
	DISP_FORMAT_YVYU = 12,

	DISP_FORMAT_XRGB_8888 = 254,
	DISP_FORMAT_UNSUPPORT = 255,
};

enum VGOP_FORMAT_E {
	VGOP_FORMAT_ARGB8888 = 0,
	VGOP_FORMAT_ARGB4444 = 4,
	VGOP_FORMAT_ARGB1555 = 5,
	VGOP_FORMAT_UNSUPPORT
};

static const struct cvitek_format disp_formats[] = {
	{ DRM_FORMAT_XRGB8888, DISP_FORMAT_XRGB_8888 },
	{ DRM_FORMAT_RGB888, DISP_FORMAT_RGB_888 },
	{ DRM_FORMAT_BGR888, DISP_FORMAT_BGR_888 },
	{ DRM_FORMAT_YUV420, DISP_FORMAT_YUV_PLANAR_420 },
	{ DRM_FORMAT_YUV422, DISP_FORMAT_YUV_PLANAR_422 },
	{ DRM_FORMAT_NV12, DISP_FORMAT_NV12 },
	{ DRM_FORMAT_NV21, DISP_FORMAT_NV21 },
	{ DRM_FORMAT_NV16, DISP_FORMAT_NV16 },
	{ DRM_FORMAT_NV61, DISP_FORMAT_NV61 },
	{ DRM_FORMAT_YUYV, DISP_FORMAT_YUYV },
	{ DRM_FORMAT_YVYU, DISP_FORMAT_YVYU },
	{ DRM_FORMAT_UYVY, DISP_FORMAT_UYVY },
	{ DRM_FORMAT_VYUY, DISP_FORMAT_VYUY },
};

static const struct cvitek_format vgop_formats[] = {
	{ DRM_FORMAT_ARGB8888, VGOP_FORMAT_ARGB8888 },
	{ DRM_FORMAT_ARGB4444, VGOP_FORMAT_ARGB4444 },
	{ DRM_FORMAT_ARGB1555, VGOP_FORMAT_ARGB1555 },
};

static const u32 primary_formats[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_RGB888, DRM_FORMAT_BGR888, DRM_FORMAT_YUV420,
	DRM_FORMAT_YUV422, DRM_FORMAT_NV12, DRM_FORMAT_NV21,
	DRM_FORMAT_NV16, DRM_FORMAT_NV61, DRM_FORMAT_YUYV,
	DRM_FORMAT_YVYU, DRM_FORMAT_UYVY, DRM_FORMAT_VYUY
};

static const u32 overlay_formats[] = {
	DRM_FORMAT_ARGB8888, DRM_FORMAT_ARGB4444, DRM_FORMAT_ARGB1555
};

struct disp_size {
	u16 w;
	u16 h;
};

struct disp_point {
	u16 x;
	u16 y;
};

struct disp_rect {
	u16 x;
	u16 y;
	u16 w;
	u16 h;
};

struct disp_csc_matrix {
	u16 coef[3][3];
	u8 sub[3];
	u8 add[3];
};

union disp_intr {
	struct {
		u32 fifo_full_error : 1;
		u32 up_1t_lite : 1;
		u32 up_1t : 1;
		u32 disp_frame_done : 1;
		u32 all_interrupt : 1;
	} b;
	u32 raw;
};

enum disp_csc {
	DISP_CSC_NONE,
	DISP_CSC_601_LIMIT_YUV2RGB,
	DISP_CSC_601_FULL_YUV2RGB,
	DISP_CSC_709_LIMIT_YUV2RGB,
	DISP_CSC_709_FULL_YUV2RGB,
	DISP_CSC_601_LIMIT_RGB2YUV,
	DISP_CSC_601_FULL_RGB2YUV,
	DISP_CSC_709_LIMIT_RGB2YUV,
	DISP_CSC_709_FULL_RGB2YUV,
	DISP_CSC_MAX,
};

enum disp_drop_mode {
	DISP_DROP_MODE_DITHER = 1,
	DISP_DROP_MODE_ROUNDING = 2,
	DISP_DROP_MODE_DROP = 3,
	DISP_DROP_MODE_MAX,
};

struct disp_mem {
	u64 addr0;
	u64 addr1;
	u64 addr2;
	u16 pitch_y;
	u16 pitch_c;
	u16 start_x;
	u16 start_y;
	u16 width;
	u16 height;
};

struct disp_gop_ow_cfg {
	enum VGOP_FORMAT_E fmt;
	struct disp_point start;
	struct disp_point end;
	u64 addr;
	u16 pitch;
	u16 crop_pixels;
	struct disp_size mem_size;
	struct disp_size img_size;
};

struct disp_gop_fb_cfg {
	union {
		struct {
			u32 width	: 7;
			u32 resv_b7	: 1;
			u32 pix_thr	: 5;
			u32 sample_rate	: 2;
			u32 resv_b15	: 1;
			u32 fb_num	: 5;
			u32 resv_b21	: 3;
			u32 attach_ow	: 3;
			u32 resv_b27	: 1;
			u32 enable	: 1;
		} b;
		u32 raw;
	} fb_ctrl;
	u32 init_st;
};

struct disp_cover_cfg {
	union {
		struct {
			u32 x       : 16;
			u32 y       : 15;
			u32 enable  : 1;
		} b;
		u32 raw;
	} start;
	struct disp_size img_size;
	union {
		struct {
			u32 cover_color_r   : 8;
			u32 cover_color_g   : 8;
			u32 cover_color_b   : 8;
			u32 resv            : 8;
		} b;
		u32 raw;
	} color;
};

struct disp_gop_odec_cfg {
	union {
		struct {
			u32 odec_en: 1;
			u32 odec_int_en: 1;
			u32 odec_int_clr: 1;
			u32 odec_dbg_ridx: 4;
			u32 odec_done: 1;
			u32 resev: 4;
			u32 odec_attached_idx: 3;
			u32 odec_int_vec: 8;
		} b;
		u32 raw;
	} odec_ctrl;
	u32 odec_debug;
};

struct disp_gop_cfg {
	union {
		struct {
			u32 ow0_en : 1;
			u32 ow1_en : 1;
			u32 ow2_en : 1;
			u32 ow3_en : 1;
			u32 ow4_en : 1;
			u32 ow5_en : 1;
			u32 ow6_en : 1;
			u32 ow7_en : 1;
			u32 hscl_en: 1;
			u32 vscl_en: 1;
			u32 colorkey_en : 1;
			u32 resv   : 1;
			u32 burst  : 4;
			u32 resv_b16 : 15;
			u32 sw_rst : 1;
		} b;
		u32 raw;
	} gop_ctrl;

	union {
		struct {
			u32 hi_thr	: 6;
			u32 resv_b6	: 2;
			u32 lo_thr	: 6;
			u32 resv_b14	: 2;
			u32 fb_init	: 1;
			u32 lo_thr_inv	: 1;
			u32 resv_b18	: 2;
			u32 detect_fnum	: 6;
		} b;
		u32 raw;
	} fb_ctrl;

	u32 colorkey;       // RGB888
	u16 font_fg_color;  // ARGB4444
	u16 font_bg_color;  // ARGB4444

	struct disp_gop_ow_cfg ow_cfg[DISP_MAX_GOP_OW_INST];
	struct disp_gop_fb_cfg fb_cfg[DISP_MAX_GOP_FB_INST];
	struct disp_gop_odec_cfg odec_cfg;
};

enum disp_pattern_mode {
	DISP_PAT_OFF = 0,
	DISP_PAT_SNOW,
	DISP_PAT_AUTO,
	DISP_PAT_RED,
	DISP_PAT_GREEN,
	DISP_PAT_BLUE,
	DISP_PAT_COLORBAR,
	DISP_PAT_GRAY_GRAD_H,
	DISP_PAT_GRAY_GRAD_V,
	DISP_PAT_BLACK,
	DISP_PAT_MAX,
};

enum disp_pat_color {
	DISP_PAT_COLOR_WHITE,
	DISP_PAT_COLOR_RED,
	DISP_PAT_COLOR_GREEN,
	DISP_PAT_COLOR_BLUE,
	DISP_PAT_COLOR_CYAN,
	DISP_PAT_COLOR_MAGENTA,
	DISP_PAT_COLOR_YELLOW,
	DISP_PAT_COLOR_BAR,
	DISP_PAT_COLOR_USR,
	DISP_PAT_COLOR_MAX
};

enum disp_pat_type {
	DISP_PAT_TYPE_FULL,
	DISP_PAT_TYPE_H_GRAD,
	DISP_PAT_TYPE_V_GRAD,
	DISP_PAT_TYPE_AUTO,
	DISP_PAT_TYPE_SNOW,
	DISP_PAT_TYPE_OFF,
	DISP_PAT_TYPE_MAX
};

struct disp_pattern {
	enum disp_pat_type type;
	enum disp_pat_color color;
	u16 rgb[3];
};

struct disp_cfg {
	bool disp_from_sc;  // 0(DRAM), 1(scaler_d)
	bool cache_mode;
	bool sync_ext;
	bool tgen_en;
	enum DISP_FORMAT_E fmt;
	enum disp_csc in_csc;
	enum disp_csc out_csc;
	u8 burst;       // 0~15
	u8 out_bit;     // 6/8/10-bit
	enum disp_drop_mode drop_mode;
	struct disp_mem mem;
	struct disp_gop_cfg gop_cfg; // gop1(0x800)
};

struct disp_timing {
	u16 width;
	u16 height;
	bool vsync_pol;
	bool hsync_pol;
	u16 htotal;
	u16 vtotal;
	u16 vsync_start;
	u16 vsync_end;
	u16 hsync_start;
	u16 hsync_end;
	u16 vfde_start;
	u16 vfde_end;
	u16 vmde_start;
	u16 vmde_end;
	u16 hfde_start;
	u16 hfde_end;
	u16 hmde_start;
	u16 hmde_end;
};

union disp_dbg_status {
	struct {
		u32 bw_fail     : 1;
		u32 bw_fail_clr : 1;
		u32 osd_bw_fail : 1;
		u32 osd_bw_fail_clr : 1;
		u32 err_fwr_y   : 1;
		u32 err_fwr_u   : 1;
		u32 err_fwr_v   : 1;
		u32 err_fwr_clr : 1;
		u32 err_erd_y   : 1;
		u32 err_erd_u   : 1;
		u32 err_erd_v   : 1;
		u32 err_erd_clr : 1;
		u32 lb_full_y   : 1;
		u32 lb_full_u   : 1;
		u32 lb_full_v   : 1;
		u32 resv1       : 1;
		u32 lb_empty_y  : 1;
		u32 lb_empty_u  : 1;
		u32 lb_empty_v  : 1;
		u32 resv2       : 13;
	} b;
	u32 raw;
};

//disp ctx
struct disp_hw_ctx {
	u32 disp_id;
	int irq;
	struct clk *disp_clk;
	struct disp_cfg disp_cfg;
	struct disp_timing disp_timing;
	bool disp_vgop_status[CVITEK_MAX_PLANE];
	spinlock_t disp_mask_spinlock;
	bool primary_formats_xr24;
};

struct cvitek_crtc {
	struct drm_crtc base;
	struct drm_pending_vblank_event *event;
	void *hw_ctx;
};

struct cvitek_plane {
	struct drm_plane base;
	void *hw_ctx;
	u32 ch;
};

struct cvitek_disp {
	struct drm_device *drm;
	struct cvitek_crtc crtc;
	struct cvitek_plane planes[CVITEK_MAX_PLANE];
	void *hw_ctx;
};

/* display controller init/cleanup ops */
struct disp_match_data {
	u32 crtc_id;
	u32 num_planes;
	u32 prim_plane;
	char irq_name[32];
	const u32 *primary_formats;
	u32 primary_formats_cnt;
	const u32 *overlay_formats;
	u32 overlay_formats_cnt;
	u32 gamma_lut_size;

	const struct drm_crtc_helper_funcs *crtc_helper_funcs;
	const struct drm_crtc_funcs *crtc_funcs;
	const struct drm_plane_helper_funcs *plane_helper_funcs;
	const struct drm_plane_funcs  *plane_funcs;

	void *(*alloc_hw_ctx)(struct platform_device *pdev);
	void (*cleanup_hw_ctx)(struct platform_device *pdev,
					void *hw_ctx);
};

// extern u16 hdmi_width;

#endif /* __CVITEK_DISP_H__ */

/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _DISP_H_
#define _DISP_H_

#include "vo_disp.h"

#define DISP_MAX_INST 2
#define DISP_MAX_GOP_INST 3
#define DISP_MAX_GOP_OW_INST 8
#define DISP_MAX_GOP_FB_INST 2
#define DISP_MAX_COVER_INST 4
#define DISP_MAX_DSI_LP 16
#define DISP_MAX_DSI_SP 2
#define MAX_OSD_ENC_INST 2
#define DISP_DEFAULT_BURST 7
#define DISP_DEFAULT_Y_THRESH 0x90
#define DISP_DEFAULT_C_THRESH 0x90
#define DISP_GOP_BURST 15
#define DISP_GAMMA_NODE 65

#define IS_YUV_FMT(x) \
	((x == DISP_FMT_YUV420) || (x == DISP_FMT_YUV422) || \
	 (x == DISP_FMT_Y_ONLY) || (x >= DISP_FMT_NV12))
#define IS_PACKED_FMT(x) \
	((x == DISP_FMT_RGB_PACKED) || (x == DISP_FMT_BGR_PACKED) || \
	 (x == DISP_FMT_YVYU) || (x == DISP_FMT_YUYV) || \
	 (x == DISP_FMT_VYUY) || (x == DISP_FMT_UYVY))

struct disp_point {
	u16 x;
	u16 y;
};

struct disp_size {
	u16 w;
	u16 h;
};
struct disp_rect {
	u16 x;
	u16 y;
	u16 w;
	u16 h;
};

enum disp_drop_mode {
	DISP_DROP_MODE_DITHER = 1,
	DISP_DROP_MODE_ROUNDING,
	DISP_DROP_MODE_DROP,
	DISP_DROP_MODE_MAX,
};

enum disp_hw_mcu_format {
	HW_MCU_FORMAT_RGB565 = 0,
	HW_MCU_FORMAT_RGB888,
	HW_MCU_FORMAT_MAX,
};

enum disp_format {
	DISP_FMT_YUV420,
	DISP_FMT_YUV422,
	DISP_FMT_RGB_PLANAR,
	DISP_FMT_BGR_PACKED, // B lsb
	DISP_FMT_RGB_PACKED, // R lsb
	DISP_FMT_Y_ONLY,
	DISP_FMT_BF16, // odma only
	DISP_FMT_NV12 = 8,
	DISP_FMT_NV21,
	DISP_FMT_YUV422SP1,
	DISP_FMT_YUV422SP2,
	DISP_FMT_YVYU,
	DISP_FMT_YUYV,
	DISP_FMT_VYUY,
	DISP_FMT_UYVY,
	DISP_FMT_MAX
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
	DISP_CSC_DATATYPE,
	DISP_CSC_MAX,
};

enum disp_gop_format {
	DISP_GOP_FMT_ARGB8888,
	DISP_GOP_FMT_ARGB4444,
	DISP_GOP_FMT_ARGB1555,
	DISP_GOP_FMT_256LUT,
	DISP_GOP_FMT_16LUT,
	DISP_GOP_FMT_FONT,
	DISP_GOP_FMT_MAX
};

enum disp_out_mode {
	DISP_OUT_CSC,
	DISP_OUT_QUANT,
	DISP_OUT_HSV,
	DISP_OUT_QUANT_BF16,
	DISP_OUT_DISABLE
};

struct disp_csc_matrix {
	u16 coef[3][3];
	u8 sub[3];
	u8 add[3];
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
	enum disp_gop_format fmt;
	struct disp_point start;
	struct disp_point end;
	u64 addr;
	u16 crop_pixels;
	u16 pitch;
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
			u32 odec_en		: 1;
			u32 odec_int_en		: 1;
			u32 odec_int_clr	: 1;
			u32 odec_wdt_en		: 1;
			u32 odec_dbg_ridx	: 4;
			u32 odec_done		: 1;
			u32 resv_b9		: 3;
			u32 odec_attached_idx	: 3;
			u32 resv_b15		: 1;
			u32 odec_wdt_fdiv_bit	: 3;
			u32 resv_b19		: 5;
			u32 odec_int_vec	: 8;
		} b;
		u32 raw;
	} odec_ctrl;
	u32 odec_debug;
	u64 bso_addr;
	u32 bso_sz; //OSD encoder original data of bso_sz
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
	u32 colorkey;       // RGB888
	u16 font_fg_color;  // ARGB4444
	u16 font_bg_color;  // ARGB4444
	struct disp_gop_ow_cfg ow_cfg[DISP_MAX_GOP_OW_INST];
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
	struct disp_gop_fb_cfg fb_cfg[DISP_MAX_GOP_FB_INST];
	struct disp_gop_odec_cfg odec_cfg;
};

struct disp_cfg {
	bool disp_from_sc;  // 0(DRAM), 1(scaler_d)
	bool cache_mode;
	bool sync_ext;
	bool tgen_en;
	enum disp_format fmt;
	enum disp_csc in_csc;
	enum disp_csc out_csc;
	u8 burst;       // 0~15
	u8 out_bit;     // 6/8/10-bit
	u8 y_thresh;
	u8 c_thresh;
	enum disp_drop_mode drop_mode;
	struct disp_mem mem;
	struct disp_gop_cfg gop_cfg[DISP_MAX_GOP_INST];
};

struct disp_oenc_cfg {
	enum disp_gop_format fmt: 4;
	union {
		struct {
			u32 fmt         : 4;
			u32 resv4       : 4;
			u32 alpha_zero  : 1;
			u32 resv3       : 3;
			u32 rgb_trunc   : 2;
			u32 alpha_trunc : 2;
			u32 limit_bsz_en: 1;
			u32 limit_bsz_bypass: 1;
			u32 wprot_en    : 1;
			u32 resv11      : 11;
			u32 wdog_en     : 1;
			u32 intr_en     : 1;
		} b;
		u32 raw;
	} cfg;
	struct disp_size src_picture_size;
	struct disp_size src_mem_size;
	u16 src_pitch;
	u64 src_adr;
	u32 wprot_laddr;
	u32 wprot_uaddr;
	u64 bso_adr;
	u32 limit_bsz;
	u32 bso_sz; //OSD encoder original data of bso_sz
	struct disp_size bso_mem_size; //for setting VGOP bitstream size
};

struct disp_oenc_int {
	union {
		struct {
			u32 go      : 1;
			u32 resv7   : 7;
			u32 done    : 1;
			u32 resv6   : 6;
			u32 intr_clr: 1;
			u32 intr_vec: 16;
		} b;
		u32 raw;
	} go_intr;
};

enum disp_pat_color {
	PAT_COLOR_WHITE,
	PAT_COLOR_RED,
	PAT_COLOR_GREEN,
	PAT_COLOR_BLUE,
	PAT_COLOR_CYAN,
	PAT_COLOR_MAGENTA,
	PAT_COLOR_YELLOW,
	PAT_COLOR_BAR,
	PAT_COLOR_USR,
	PAT_COLOR_MAX
};

union disp_vo_mux_sel {
	struct {
		u32 vo_sel_type	: 4;
	} b;
	u32 raw;
};

union disp_vo_mux {
	struct {
		u32 vod_sel0	: 8;
		u32 vod_sel1	: 8;
		u32 vod_sel2	: 8;
		u32 vod_sel3	: 8;
	} b;
	u32 raw;
};

struct disp_quant_formula {
	u16 sc_frac[3];
	u8 sub[3];
	u16 sub_frac[3];
};

struct disp_convertto_formula {
	u32 a_frac[3];
	u32 b_frac[3];
};

enum disp_hsv_rounding {
	DISP_HSV_ROUNDING_AWAY_FROM_ZERO = 0,
	DISP_HSV_ROUNDING_TOWARD_ZERO,
	DISP_HSV_ROUNDING_MAX,
};

enum disp_quant_rounding {
	DISP_QUANT_ROUNDING_TO_EVEN = 0,
	DISP_QUANT_ROUNDING_AWAY_FROM_ZERO = 1,
	DISP_QUANT_ROUNDING_TRUNCATE = 2,
	DISP_QUANT_ROUNDING_MAX,
};

struct disp_csc_cfg {
	enum disp_out_mode mode;
	union {
		enum disp_csc csc_type;
		struct disp_quant_formula quant_form;
	};
	union {
		enum disp_hsv_rounding hsv_round;
		enum disp_quant_rounding quant_round;
	};
};

struct disp_odma_cfg {
	bool burst;     // burst(0: 8, 1:16)
	bool enable;
	enum disp_format fmt;
	struct disp_mem mem;
	enum disp_csc csc;
};

enum disp_img_trig_src {
	DISP_IMG_TRIG_SRC_SW = 0,
	DISP_IMG_TRIG_SRC_DISP,
	DISP_IMG_TRIG_SRC_ISP,
	DISP_IMG_TRIG_SRC_MAX,
};

union disp_intr {
	struct {
		u32 disp_odma_fifo_full_err : 1; //0
		u32 disp_tgen_lite : 1;
		u32 disp_frame_start : 1;
		u32 disp_frame_end : 1;
		u32 disp_all_intr : 1;
		u32 resv_27 : 27;
	} b;
	u32 raw;
};

union disp_odma_intr_sel {
	struct {
		u32 resv0_8 : 8; //0
		u32 disp_online_frame_end : 1;
		u32 disp_odma_frame_end : 1;
		u32 resv1_22 : 22;
	} b;
	u32 raw;
};

union disp_intr_sel {
	struct {
		u32 disp_frame_end : 1; //0
		u32 disp_frame_start : 1;
		u32 disp_tgen_lite : 1;
		u32 disp_odma_fifo_full : 1;
		u32 resv0_28 : 28;
	} b;
	u32 raw;
};

union disp_intr_clr {
	struct {
		u32 disp_frame_end : 1; //0
		u32 disp_frame_start : 1;
		u32 disp_tgen_lite : 1;
		u32 resv0_29 : 29;
	} b;
	u32 raw;
};

enum disp_pat_type {
	PAT_TYPE_FULL,
	PAT_TYPE_H_GRAD,
	PAT_TYPE_V_GRAD,
	PAT_TYPE_AUTO,
	PAT_TYPE_SNOW,
	PAT_TYPE_OFF,
	PAT_TYPE_MAX
};

/**
 * @ vsync_pol: vsync polarity
 * @ hsync_pol: hsync polarity
 * @ vtotal: total line of each frame, should sub 1,
 *           start line is included, end line isn't included
 * @ htotal: total pixel of each line, should sub 1,
 *           start pixel is included, end pixel isn't included
 * @ vsync_start: start line of vsync
 * @ vsync_end: end line of vsync, should sub 1
 * @ vfde_start: start line of actually video data
 * @ vfde_end: end line of actually video data, should sub 1
 * @ vmde_start: equal to vfde_start
 * @ vmde_end: equal to vfde_end
 * @ hsync_start: start pixel of hsync
 * @ hsync_end: end pixel of hsync, should sub 1
 * @ hfde_start: start pixel of actually video data in each line
 * @ hfde_end: end pixel of actually video data in each line, should sub 1
 * @ hmde_start: equal to hfde_start
 * @ hmde_end: equal to hfde_end
 */
struct disp_timing {
	bool vsync_pol;
	bool hsync_pol;
	u16 vtotal;
	u16 htotal;
	u16 vsync_start;
	u16 vsync_end;
	u16 vfde_start;
	u16 vfde_end;
	u16 vmde_start;
	u16 vmde_end;
	u16 hsync_start;
	u16 hsync_end;
	u16 hfde_start;
	u16 hfde_end;
	u16 hmde_start;
	u16 hmde_end;
};

union disp_mcu_if_ctrl {
	struct {
		u32 i80_if_en	: 1;
		u32 i80_sw_mode_en	: 1;
		u32 i80_hw_if_en	: 1;
		u32 resv	: 29;
	} b;
	u32 raw;
};

union disp_hw_mcu_auto {
	struct {
		u32 mcu_hw_trig	: 1;
		u32 mcu_hw_stop	: 1;
		u32 cs_h_hw_blk	: 2;
		u32 mcu_565	: 1;
		u32 resv	: 27;
	} b;
	u32 raw;
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

struct disp_checksum_status {
	union{
		struct {
			u32 data_in_from_sc_d   : 8;
			u32 data_out            : 8;
			u32 reserv              : 15;
			u32 enable              : 1;
		} b;
		u32 raw;
	} checksum_base;
	u32 axi_read_from_dram;
	u32 axi_read_from_gop;
};

/**
 * @ out_bit: 0(6-bit), 1(8-bit), others(10-bit)
 * @ vesa_mode: 0(JEIDA), 1(VESA)
 * @ dual_ch: dual link
 * @ vs_out_en: vs output enable
 * @ hs_out_en: hs output enable
 * @ hs_blk_en: vertical blanking hs output enable
 * @ ml_swap: lvdstx hs data msb/lsb swap
 * @ ctrl_rev: serializer 0(msb first), 1(lsb first)
 * @ oe_swap: lvdstx even/odd link swap
 * @ en: lvdstx enable
 */
union disp_lvdstx {
	struct {
		u32 out_bit	: 2;
		u32 vesa_mode	: 1;
		u32 dual_ch	: 1;
		u32 vs_out_en	: 1;
		u32 hs_out_en	: 1;
		u32 hs_blk_en	: 1;
		u32 resv_1	: 1;
		u32 ml_swap	: 1;
		u32 ctrl_rev	: 1;
		u32 oe_swap	: 1;
		u32 en		: 1;
		u32 resv	: 20;
	} b;
	u32 raw;
};

/**
 * @fmt_sel: [0] clk select
 *		0: bt clock 2x of disp clock
 *		1: bt clock 2x of disp clock
 *	     [1] sync signal index
 *		0: with sync pattern
 *		1: without sync pattern
 * @hde_gate: gate output hde with vde
 * @data_seq: fmt_sel[0] = 0
 *		00: Cb0Y0Cr0Y1
 *		01: Cr0Y0Cb0Y1
 *		10: Y0Cb0Y1Cr0
 *		11: Y0Cr0Y1Cb0
 *	      fmt_sel[0] = 1
 *		0: Cb0Cr0
 *		1: Cr0Cb0
 * @clk_inv: clock rising edge at middle of data
 * @vs_inv: vs low active
 * @hs_inv: hs low active
 */
union disp_bt_enc {
	struct {
		u32 fmt_sel	: 2;
		u32 resv_1	: 1;
		u32 hde_gate	: 1;
		u32 data_seq	: 2;
		u32 resv_2	: 2;
		u32 clk_inv	: 1;
		u32 hs_inv	: 1;
		u32 vs_inv	: 1;
	} b;
	u32 raw;
};

/**
 * @ sav_vld: sync pattern for start of valid data
 * @ sav_blk: sync pattern for start of blanking data
 * @ eav_vld: sync pattern for end of valid data
 * @ eav_blk: sync pattern for end of blanking data
 */
union disp_bt_sync_code {
	struct {
		u8 sav_vld;
		u8 sav_blk;
		u8 eav_vld;
		u8 eav_blk;
	} b;
	u32 raw;
};

union disp_srgb_ctrl {
	struct {
		u32 srgb_ttl_en	: 1;
		u32 srgb_ttl_4t	: 1;
		u32 resv	: 30;
	} b;
	u32 raw;
};

enum disp_vo_sel {
	DISP_VO_SEL_DISABLE,
	DISP_VO_SEL_RGB,
	DISP_VO_SEL_SW,
	DISP_VO_SEL_I80,
	DISP_VO_SEL_BT601,
	DISP_VO_SEL_BT656,
	DISP_VO_SEL_BT1120,
	DISP_VO_SEL_BT1120R,
	DISP_VO_SEL_SERIAL_RGB,
	DISP_VO_SEL_HW_MCU,
	DISP_VO_SEL_MAX,
};

enum disp_dsi_mode {
	DISP_DSI_MODE_IDLE = 0,
	DISP_DSI_MODE_SPKT = 1,
	DISP_DSI_MODE_ESC = 2,
	DISP_DSI_MODE_HS = 4,
	DISP_DSI_MODE_UNKNOWN,
	DISP_DSI_MODE_MAX = DISP_DSI_MODE_UNKNOWN,
};

enum disp_dsi_fmt {
	DISP_DSI_FMT_RGB888 = 0,
	DISP_DSI_FMT_RGB666,
	DISP_DSI_FMT_RGB565,
	DISP_DSI_FMT_RGB101010,
	DISP_DSI_FMT_MAX,
};

struct disp_privacy_map_cfg {
	u32 base;
	u16 axi_burst;
	u8 no_mask_idx;
	u16 alpha_factor;
};

struct disp_privacy_cfg {
	union {
		struct {
			u32 enable	: 1;
			u32 mode	: 1; // 0 : grid mode, 1 : pixel mode
			u32 force_alpha	: 1; // 0 : depend on no_mask_idx, 1: force alpha
			u32 mask_rgb332	: 1; // 0 : y, 1: rgb332
			u32 blend_y	: 1; // blending on Y/R only
			u32 y2r_enable	: 1; // apply y2r csc on output
			u32 grid_size	: 1; // 0 : 8x8, 1: 16x16
			u32 fit_picture	: 1; // 0: customized size, 1: same size as sc_core
		} b;
		u32 raw;
	} cfg;
	struct disp_point start;
	struct disp_point end;
	struct disp_privacy_map_cfg map_cfg;
};

/**
 * @ enable: gamma enbale
 * @ pre_osd: 0:osd-->gamma 1:gamma-->osd
 * @ r: LUT gamma red
 * @ g: LUT gamma green
 * @ b: LUT gamma blue
 */
struct disp_gamma_attr {
	bool enable;
	bool pre_osd;
	u8  table[DISP_GAMMA_NODE];
};

void disp_ctrl_init(bool is_resume);
void disp_set_intr_mask(u8 inst, union disp_intr_sel disp_intr);
void disp_get_intr_mask(u8 inst, union disp_intr_sel *disp_intr);
void disp_set_odma_intr_mask(u8 inst, union disp_odma_intr_sel online_odma_mask);
void disp_get_odma_intr_mask(u8 inst, union disp_odma_intr_sel *online_odma_mask);

void disp_intr_clr(u8 inst, union disp_intr_clr disp_intr);
void disp_odma_fifofull_clr(u8 inst);

union disp_intr disp_intr_status(u8 inst);
union disp_dbg_status disp_get_dbg_status(u8 inst, bool clr);

void disp_set_vo_mac_base_addr(u8 inst, void *base);
void disp_set_disp_base_addr(u8 inst, void *base);
void disp_set_dsi_mac_base_addr(u8 inst, void *base);
void disp_set_oenc_base_addr(u8 inst, void *base);
void disp_top_set_vo_data_mux(u8 inst, u8 vodata_selID, u8 value);
enum disp_vo_sel disp_mux_get(u8 inst);
void disp_set_vo_type_sel(u8 inst, enum disp_vo_sel vo_sel);

void disp_reg_shadow_sel(u8 inst, bool read_shadow);
void disp_reg_force_up(u8 inst);

void disp_set_bw_cfg(u8 inst, enum disp_format fmt);
void disp_set_cfg(u8 inst, struct disp_cfg *cfg);
struct disp_cfg *disp_get_cfg(u8 inst);
int disp_set_rect(u8 inst, struct disp_rect rect);
void disp_set_mem(u8 inst, struct disp_mem *mem);
void disp_set_addr(u8 inst, u64 addr0, u64 addr1, u64 addr2);
void _disp_set_in_csc(u8 inst, struct disp_csc_matrix *cfg);
void disp_set_in_csc(u8 inst, enum disp_csc csc);
void disp_set_out_csc(u8 inst, enum disp_csc csc);
void disp_set_pattern(u8 inst, enum disp_pat_type type,
			   enum disp_pat_color color, const u16 *rgb);
void disp_set_frame_bgcolor(u8 inst, u16 r, u16 g, u16 b);
void disp_set_window_bgcolor(u8 inst, u16 r, u16 g, u16 b);
void disp_enable_window_bgcolor(u8 inst, bool enable);
void disp_clk_enable(u8 inst, bool enable);
bool disp_tgen_enable(u8 inst, bool enable);
bool disp_check_tgen_enable(u8 inst);
bool disp_check_i80_enable(u8 inst);

void disp_bt_set(u8 inst, union disp_bt_enc enc, union disp_bt_sync_code sync);
void disp_bt_get(u8 inst, union disp_bt_enc *enc, union disp_bt_sync_code *sync);
void disp_bt_en(u8 inst);
void disp_vo_mux_sel(u8 inst, int vo_sel, int vo_mux);
void disp_timing_setup_from_reg(u8 inst);
void disp_cfg_setup_from_reg(u8 inst);
void disp_checksum_en(u8 inst, bool enable);
void disp_get_checksum_status(u8 inst, struct disp_checksum_status *status);

enum disp_dsi_mode dsi_get_mode(u8 inst);
int dsi_set_mode(u8 inst, enum disp_dsi_mode mode);
void disp_dsi_clr_mode(u8 inst);
int dsi_chk_mode_done(u8 inst, enum disp_dsi_mode mode);
int dsi_long_packet(u8 inst, u8 di, const u8 *data, u8 count, bool sw_mode);
int dsi_long_packet_raw(u8 inst, const u8 *data, u8 count);
int dsi_short_packet(u8 inst, u8 di, const u8 *data, u8 count, bool sw_mode);
int dsi_dcs_write_buffer(u8 inst, u8 di, const void *data, size_t len, bool sw_mode);
int dsi_dcs_read_buffer(u8 inst, u8 di, const u16 data_param, u8 *data, size_t len);
int disp_dsi_config(u8 inst, u8 lane_num, enum disp_dsi_fmt fmt, u16 width);

void disp_set_srgb_ttl_en(u8 inst, bool enable);
void disp_set_srgb_ttl_4x(u8 inst, bool is_4x);

void disp_gamma_ctrl(u8 inst, bool enable, bool pre_osd);
void disp_gamma_lut_update(u8 inst, const u8 *b, const u8 *g, const u8 *r);
void disp_gamma_lut_read(u8 inst, struct disp_gamma_attr *gamma_attr);

int ctrl_set_disp_src(u8 inst, bool disp_from_sc);

void disp_set_intf(u8 inst, enum vo_disp_intf intf);

void disp_lvdstx_set(u8 inst, union disp_lvdstx cfg);
void disp_lvdstx_get(u8 inst, union disp_lvdstx *cfg);

void disp_oenc_set_cfg(u8 inst, struct disp_oenc_cfg *oenc_cfg);
struct disp_oenc_cfg *disp_oenc_get_cfg(u8 inst);

void disp_gop_set_cfg(u8 inst, u8 layer, struct disp_gop_cfg *cfg, bool update);
struct disp_gop_cfg *disp_gop_get_cfg(u8 inst, u8 layer);
void disp_gop_ow_set_cfg(u8 inst, u8 layer, u8 ow_inst, struct disp_gop_ow_cfg *cfg, bool update);
int disp_gop_setup_256LUT(u8 inst, u8 layer, u16 length, u16 *data);
int disp_gop_update_256LUT(u8 inst, u8 layer, u16 index, u16 data);
int disp_gop_setup_16LUT(u8 inst, u8 layer, u8 length, u16 *data);
int disp_gop_update_16LUT(u8 inst, u8 layer, u8 index, u16 data);
void disp_gop_fb_set_cfg(u8 inst, u8 layer, u8 fb_inst, struct disp_gop_fb_cfg *cfg);
u32 disp_gop_fb_get_record(u8 inst, u8 layer, u8 fb_inst);
void disp_gop_ow_get_addr(u8 inst, u8 layer, u8 ow_inst, u64 *addr);
void disp_gop_odec_set_cfg_from_oenc(u8 inst, u8 layer, u8 oenc_inst,
					struct disp_gop_odec_cfg *odec_cfg);
void disp_cover_set_cfg(u8 inst, u8 cover_w_inst, struct disp_cover_cfg *cover_cfg);

void disp_set_timing(u8 inst, struct disp_timing *timing);
struct disp_timing *disp_get_timing(u8 inst);
void disp_get_hw_timing(u8 inst, struct disp_timing *timing);

// odma
void disp_odma_set_addr(u8 inst, u64 addr0, u64 addr1, u64 addr2);
void disp_odma_set_mem(u8 inst, struct disp_mem *mem);
void disp_odma_set_cfg(u8 inst, struct disp_odma_cfg *cfg);
struct disp_odma_cfg *disp_odma_get_cfg(u8 inst);
void disp_odma_set_fmt(u8 inst, enum disp_format fmt);
void disp_odma_enable(u8 inst, bool enable);

bool ddr_need_retrain(void);
void trigger_8051(void);

#endif  //_DISP_H_

#ifndef _SCL_H_
#define _SCL_H_

#include <linux/defines.h>

#define SCL_MAX_INST VPSS_IP_NUM
#define BORDER_VPP_MAX 4
#define COVER_MAX 4
#define SCL_MAX_DSI_LP 16
#define SCL_MAX_DSI_SP 2
#define SCL_MAX_GOP_INST 2
#define SCL_MAX_GOP_OW_INST 8
#define SCL_MAX_GOP_FB_INST 2
#define SCL_MAX_COVER_INST 4

#define IS_YUV_FMT(x) \
	((x == SCL_FMT_YUV420) || (x == SCL_FMT_YUV422) || \
	 (x == SCL_FMT_Y_ONLY) || (x >= SCL_FMT_NV12))

#define IS_PACKED_FMT(x) \
	((x == SCL_FMT_RGB_PACKED) || (x == SCL_FMT_BGR_PACKED) || \
	 (x == SCL_FMT_YVYU) || (x == SCL_FMT_YUYV) || \
	 (x == SCL_FMT_VYUY) || (x == SCL_FMT_UYVY))

#define TILE_ON_IMG
#define TILE_GUARD_PIXEL 260

#define IMG_IN_PITCH_ALIGN 32

#define SCL_TILE_LEFT		(1 << 0)
#define SCL_TILE_RIGHT		(1 << 1)
#define SCL_TILE_TOP		(1 << 2)
#define SCL_TILE_DOWN		(1 << 3)
#define SCL_TILE_BOTH		(SCL_TILE_LEFT | SCL_TILE_RIGHT)
#define SCL_V_TILE_BOTH		(SCL_TILE_TOP | SCL_TILE_DOWN)
#define SCL_RIGHT_DOWN_TILE_FLAG	(BIT(6))
#define SCL_V_TILE_OFFSET 			2

struct sclr_size {
	u16 w;
	u16 h;
};

struct sclr_point {
	u16 x;
	u16 y;
};

struct sclr_rect {
	u16 x;
	u16 y;
	u16 w;
	u16 h;
};

struct sclr_status {
	u8 crop_idle : 1;
	u8 hscale_idle : 1;
	u8 vscale_idle : 1;
	u8 gop_idle : 1;
	u8 wdma_idle : 1;
	u8 rsv : 2;
};

enum vpss_dev{
	VPSS_V0 = 0,
	VPSS_V1,
	VPSS_V2,
	VPSS_V3,
	VPSS_T0,
	VPSS_T1,
	VPSS_T2,
	VPSS_T3,
	VPSS_D0,
	VPSS_D1,
	VPSS_MAX,
};

enum sclr_input {
	SCL_INPUT_ISP,
	SCL_INPUT_DWA,
	SCL_INPUT_SHARE, //dma 1ton
	SCL_INPUT_MEM,
	SCL_INPUT_MAX,
};
enum sclr_cir_mode {
	SCL_CIR_EMPTY,
	SCL_CIR_SHAPE = 2,
	SCL_CIR_LINE,
	SCL_CIR_DISABLE,
	SCL_CIR_MAX,
};

struct sclr_csc_matrix {
	u16 coef[3][3];
	u8 sub[3];
	u8 add[3];
};

struct sclr_quant_formula {
	u16 sc_frac[3];
	u8 sub[3];
	u16 sub_frac[3];
};

struct sclr_convertto_formula {
	u32 a_frac[3];
	u32 b_frac[3];
};

union sclr_intr {
	struct {
		u32 reserve1                   : 4;
		//bit4-7
		u32 img_in_frame_start         : 1;
		u32 img_in_frame_end           : 1;
		u32 reserve2                   : 1;
		u32 scl_frame_end              : 1;
		//bit8-11
		u32 reserve3                   : 2;
		u32 prog_too_late              : 1;
		u32 reserve4                   : 1;
		//bit12-15
		u32 scl_line_target_hit        : 1;
		u32 reserve5                   : 3;
		//bit16-27
		u32 scl_cycle_line_target_hit  : 1;
		u32 reserve6                   : 10;
		u32 cmdq                       : 1;
		//bit28-31
		u32 cmdq_start                 : 1;
		u32 cmdq_end                   : 1;
		u32 cmdq_lint_hit              : 1;
		u32 cmdq_cycle_line_hit        : 1;
	} b;
	u32 raw;
};

enum sclr_format {
	SCL_FMT_YUV420,
	SCL_FMT_YUV422,
	SCL_FMT_RGB_PLANAR,
	SCL_FMT_BGR_PACKED, // B lsb
	SCL_FMT_RGB_PACKED, // R lsb
	SCL_FMT_Y_ONLY,
	SCL_FMT_BF16, // odma only
	SCL_FMT_FBC,
	SCL_FMT_NV12 = 8,
	SCL_FMT_NV21,
	SCL_FMT_YUV422SP1,
	SCL_FMT_YUV422SP2,
	SCL_FMT_YVYU,
	SCL_FMT_YUYV,
	SCL_FMT_VYUY,
	SCL_FMT_UYVY,
	SCL_FMT_MAX
};

enum sclr_out_mode {
	SCL_OUT_CSC,
	SCL_OUT_QUANT,
	SCL_OUT_HSV,
	SCL_OUT_CONVERT_TO,
	SCL_OUT_DISABLE
};

enum sclr_csc {
	SCL_CSC_NONE,
	SCL_CSC_601_LIMIT_YUV2RGB,
	SCL_CSC_601_FULL_YUV2RGB,
	SCL_CSC_709_LIMIT_YUV2RGB,
	SCL_CSC_709_FULL_YUV2RGB,
	SCL_CSC_601_LIMIT_RGB2YUV,
	SCL_CSC_601_FULL_RGB2YUV,
	SCL_CSC_709_LIMIT_RGB2YUV,
	SCL_CSC_709_FULL_RGB2YUV,
	SCL_CSC_DATATYPE,
	SCL_CSC_MAX,
};

enum sclr_algorithm {
	SCL_COEF_BICUBIC = 0,
	SCL_COEF_BILINEAR,
	SCL_COEF_NEAREST,
	SCL_COEF_BICUBIC_OPENCV,
	SCL_COEF_MAX
};


enum sclr_hsv_rounding {
	SCL_HSV_ROUNDING_AWAY_FROM_ZERO = 0,
	SCL_HSV_ROUNDING_TOWARD_ZERO,
	SCL_HSV_ROUNDING_MAX,
};

enum sclr_quant_rounding {
	SCL_QUANT_ROUNDING_TO_EVEN = 0,
	SCL_QUANT_ROUNDING_AWAY_FROM_ZERO = 1,
	SCL_QUANT_ROUNDING_TRUNCATE = 2,
	SCL_QUANT_ROUNDING_MAX,
};

enum sclr_odma_datatype {
	SCL_DATATYPE_DISABLE,
	SCL_DATATYPE_U8,
	SCL_DATATYPE_INT8,
	SCL_DATATYPE_FP16,
	SCL_DATATYPE_FP32,
	SCL_DATATYPE_BF16,
};

struct sclr_csc_cfg {
	enum sclr_out_mode mode;
	enum sclr_odma_datatype datatype;
	bool work_on_border;
	union {
		enum sclr_csc csc_type;
		struct sclr_quant_formula quant_form;
	};
	struct sclr_convertto_formula convert_to_cfg;
	union {
		enum sclr_hsv_rounding hsv_round;
		enum sclr_quant_rounding quant_round;
	};
	enum sclr_quant_border_type {
		SCL_QUANT_BORDER_TYPE_255 = 0, // 0 ~ 255
		SCL_QUANT_BORDER_TYPE_127, // -128 ~ 127
		SCL_QUANT_BORDER_TYPE_MAX,
	} quant_border_type;
	enum sclr_quant_gain_mode {
		SCL_QUANT_GAIN_MODE_1 = 0, // max 8191/8192
		SCL_QUANT_GAIN_MODE_2, // max 1 + 4095/4096
		SCL_QUANT_GAIN_MAX,
	} quant_gain_mode;
};

enum sclr_flip_mode {
	SCL_FLIP_NO,
	SCL_FLIP_HFLIP,
	SCL_FLIP_VFLIP,
	SCL_FLIP_HVFLIP,
	SCL_FLIP_MAX
};

enum sclr_gop_format {
	SCL_GOP_FMT_ARGB8888,
	SCL_GOP_FMT_ARGB4444,
	SCL_GOP_FMT_ARGB1555,
	SCL_GOP_FMT_256LUT,
	SCL_GOP_FMT_16LUT,
	SCL_GOP_FMT_FONT,
	SCL_GOP_FMT_MAX
};

enum sclr_img_trig_src {
	SCL_IMG_TRIG_SRC_SW = 0,
	SCL_IMG_TRIG_SRC_DISP,
	SCL_IMG_TRIG_SRC_ISP,
	SCL_IMG_TRIG_SRC_MAX,
};

union sclr_top_cfg_01 {
	struct {
		u32 reserve1      : 1;    // 0
		u32 sc_enable     : 1;
		u32 reserve2      : 3;
		u32 fbd_enable	  : 1;    // 5
		u32 reserve3      : 6;
		u32 sc_debug_en   : 1;    // 12
		u32 reserve4      : 3;
		u32 qos_en        : 8;    // 16
	} b;
	u32 raw;
};

/*
 * reg_sb_wr_ctrl0:
 *   slice buffer push signal (sb_push_128t[0]) to VENC config
 *   0 : sc_d
 *   1 : sc_v1
 *   2 : sc_v2
 *   3 : sc_v3
 *
 * reg_sb_wr_ctrl1:
 *   slice buffer push signal (sb_push_128t[1]) to VENC config
 *   0 : sc_d
 *   1 : sc_v1
 *   2 : sc_v2
 *   3 : sc_v3
 *
 * reg_sb_rd_ctrl:
 *   slice buffer pop signal (sb_pop_128t) to LDC config
 *   0 : img_in_d
 *   1 : img_in_v
 */
struct sclr_top_sb_cfg {
	u8 sb_wr_ctrl0;
	u8 sb_wr_ctrl1;
	u8 sb_rd_ctrl;
};

struct sclr_bld_cfg {
	union {
		struct {
			u32 enable	 : 1;
			u32 fix_alpha	 : 1; // 0 : gradient alpha with x position (stiching)
					      // 1 : fix alpha value , blending setting in alpha_factor (blending)
			u32 blend_y	 : 1; // blending on Y/R only
			u32 y2r_enable	 : 1; // apply y2r csc on output
			u32 resv_b4	 : 4;
			u32 alpha_factor : 9; // blending factor
		} b;
		u32 raw;
	} ctrl;
};

union sclr_rt_cfg {
	struct {
		u32 sc_d_rt	: 1;
		u32 sc_v_rt	: 1;
		u32 sc_rot_rt	: 1;
		u32 img_d_rt	: 1;
		u32 img_v_rt	: 1;
		u32 img_d_sel	: 1;
	} b;
	u32 raw;
};

struct sclr_img_in_sb_cfg {
	u8 sb_mode;
	u8 sb_size;
	u8 sb_nb;
	u8 sb_sw_rptr;
	u8 sb_set_str;
	u8 sb_sw_clr;
};

struct sclr_mem {
	u64 addr0;
	u64 addr1;
	u64 addr2;
	u32 pitch_y;
	u32 pitch_c;
	u16 start_x;
	u16 start_y;
	u16 width;
	u16 height;
};

struct sclr_img_cfg {
	u8 src;      // 0(ISP), 1(dwa), 2(share), 3(DRAM)
	bool dup2fancy_enable;
	bool csc_en;
	enum sclr_csc csc;
	u8 burst;       // 0~15
	u8 outstanding;
	u8 fifo_y;
	u8 fifo_c;
	enum sclr_format fmt;
	struct sclr_mem mem;
};

union sclr_img_dbg_status {
	struct {
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
		u32 resv2       : 1;
		u32 ip_idle     : 1;
		u32 ip_int      : 1;
		u32 ip_clr      : 1;
		u32 ip_int_clr  : 1;
		u32 resv        : 13;
	} b;
	u32 raw;
};

struct sclr_img_checksum_status {
	union{
		struct {
			u32 output_data : 8;
			u32 reserv      : 23;
			u32 enable      : 1;
		} b;
		u32 raw;
	} checksum_base;
	u32 axi_read_data;
};

struct sclr_cir_cfg {
	enum sclr_cir_mode mode;
	u8 line_width;
	struct sclr_point center;
	u16 radius;
	struct sclr_rect rect;
	u8 color_r;
	u8 color_g;
	u8 color_b;
};

struct sclr_border_cfg {
	union {
		struct {
			u32 bd_color_r	: 8;
			u32 bd_color_g	: 8;
			u32 bd_color_b	: 8;
			u32 resv	: 7;
			u32 enable	: 1;
		} b;
		u32 raw;
	} cfg;
	struct sclr_point start;
};

struct sclr_border_vpp_cfg {
	union {
		struct {
			u32 bd_color_r	: 8;
			u32 bd_color_g	: 8;
			u32 bd_color_b	: 8;
			u32 resv	: 7;
			u32 enable	: 1;
		} b;
		u32 raw;
	} cfg;
	struct sclr_point inside_start;
	struct sclr_point inside_end;
	struct sclr_point outside_start;
	struct sclr_point outside_end;
};

struct sclr_oenc_int {
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
	}go_intr;
};

struct sclr_oenc_cfg {
	enum sclr_gop_format fmt: 4;
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
	struct sclr_size src_picture_size;
	struct sclr_size src_mem_size;
	u16 src_pitch;
	u64 src_adr;
	u32 wprot_laddr;
	u32 wprot_uaddr;
	u64 bso_adr;
	u32 limit_bsz;
	u32 bso_sz; //OSD encoder original data of bso_sz
	struct sclr_size bso_mem_size; //for setting VGOP bitstream size
};

struct sclr_gop_ow_cfg {
	enum sclr_gop_format fmt;
	struct sclr_point start;
	struct sclr_point end;
	u64 addr;
	u16 crop_pixels;
	u16 pitch;
	struct sclr_size mem_size;
	struct sclr_size img_size;
};

struct sclr_gop_fb_cfg {
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

struct sclr_gop_odec_cfg {
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

struct sclr_cover_cfg {
    union {
        struct {
            u32 x       : 16;
            u32 y       : 15;
            u32 enable  : 1;
        }b;
        u32 raw;
    } start;
    struct sclr_size img_size;
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

struct sclr_gop_cfg {
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
	struct sclr_gop_ow_cfg ow_cfg[SCL_MAX_GOP_OW_INST];
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
	struct sclr_gop_fb_cfg fb_cfg[SCL_MAX_GOP_FB_INST];
	struct sclr_gop_odec_cfg odec_cfg;
};

struct sclr_odma_sb_cfg {
	u8 sb_mode;
	u8 sb_size;
	u8 sb_nb;
	u8 sb_full_nb;
	u8 sb_sw_wptr;
	u8 sb_set_str;
	u8 sb_sw_clr;
};

struct sclr_odma_cfg {
	enum sclr_flip_mode flip;
	bool burst;     // burst(0: 8, 1:16)
	enum sclr_format fmt;
	struct sclr_mem mem;
	struct sclr_csc_cfg csc_cfg;
	struct sclr_size frame_size;
};

union sclr_odma_dbg_status {
	struct {
		u32 axi_status  : 4;
		u32 v_buf_empty : 1;
		u32 v_buf_full  : 1;
		u32 u_buf_empty : 1;
		u32 u_buf_full  : 1;
		u32 y_buf_empty : 1;
		u32 y_buf_full  : 1;
		u32 v_axi_active: 1;
		u32 u_axi_active: 1;
		u32 y_axi_active: 1;
		u32 axi_active  : 1;
		u32 resv        : 18;
	} b;
	u32 raw;
};

struct sclr_tile_cfg {
	struct sclr_size in_mem;
	struct sclr_size src;
	struct sclr_size out;
	//u16 src_l_offset;
	u16 src_l_width;
	u16 src_r_offset;
	u16 src_r_width;
	u32 r_ini_phase;
	u16 out_l_width;
	u16 dma_l_x;
	u16 dma_l_y;
	u16 dma_r_x;
	u16 dma_r_y;
	u16 dma_l_width;
	bool border_enable;
};

struct sclr_fac_cfg {
	u32 h_fac;
	u32 v_fac;
	u32 h_pos;
	u32 v_pos;
};

struct sclr_scale_cfg {
	struct sclr_size src;
	struct sclr_rect crop;
	struct sclr_size dst;
	struct sclr_tile_cfg tile;
	struct sclr_tile_cfg v_tile;
	enum sclr_algorithm algorithm;
	struct sclr_fac_cfg fac;
	bool mir_enable  : 1;
	bool tile_enable : 1;
	bool v_tile_enable : 1;
};

struct sclr_scale_2tap_cfg {
	u32 src_wd;
	u32 src_ht;
	u32 dst_wd;
	u32 dst_ht;

	u32 area_fast;
	u32 scale_x;
	u32 scale_y;
	u32 h_nor;
	u32 v_nor;
	u32 h_ph;
	u32 v_ph;
};

struct sclr_core_cfg {
	union {
		struct {
			u8 resv_b0	: 1;
			u8 sc_bypass	: 1;
			u8 gop_bypass	: 1;
			u8 resv_b3	: 1;
			u8 dbg_en	: 1;
			u8 cir_bypass	: 1;
			u8 odma_bypass	: 1;
		};
		u8 raw;
	};
	struct sclr_scale_cfg sc;
	struct sclr_tile_cfg tile;
	enum sclr_algorithm coef;
	struct sclr_cover_cfg cover_cfg[SCL_MAX_COVER_INST];
};

struct sclr_core_checksum_status {
	union{
		struct {
			u32 data_in_from_img_in : 8;
			u32 data_out            : 8;
			u32 reserv              : 15;
			u32 enable              : 1;
		} b;
		u32 raw;
	} checksum_base;
	u32 axi_read_gop0_data;
	u32 axi_read_gop1_data;
	u32 axi_write_data;
};

struct sclr_ctrl_cfg {
	enum vpss_dev img_inst;
	enum sclr_input input;
	enum sclr_format src_fmt;
	enum sclr_csc src_csc;
	struct sclr_size src;
	struct sclr_rect src_crop;
	u32 pitch_y;
	u32 pitch_c;
	u64 src_addr0;
	u64 src_addr1;
	u64 src_addr2;

	struct {
		u8 inst;
		enum sclr_format fmt;
		enum sclr_csc csc;
		enum sclr_flip_mode flip_mode;
		struct sclr_size frame;
		struct sclr_size window;
		struct sclr_point offset;
		u32 pitch_y;
		u32 pitch_c;
		u64 addr0;
		u64 addr1;
		u64 addr2;
	} dst;

	struct sclr_border_cfg border_cfg;
	struct sclr_border_vpp_cfg border_vpp_cfg[BORDER_VPP_MAX];
};

struct sclr_privacy_map_cfg {
	u64 base;
	u16 axi_burst;
	u8 no_mask_idx;
	u16 alpha_factor;
};

struct sclr_privacy_cfg {
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
	struct sclr_point start;
	struct sclr_point end;
	struct sclr_privacy_map_cfg map_cfg;
};

struct sclr_fbd_cfg {
	bool enable;
	struct sclr_rect crop;
	u64 offset_base_y;
	u64 offset_base_c;
	u64 comp_base_y;
	u64 comp_base_c;
	u16 height_y;
	u16 height_c;
	u16 stride_y;
	u16 stride_c;
	u8 depth_y;//8bit:0, 9bit:1, 10bit:2
	u8 depth_c;//8bit:0, 9bit:1, 10bit:2
	bool mono_en;//monochrome:1 otherwise:0
	bool otbg_64x64_en;//when AV1 Sequence header screen_content_tools is 1, this should be set to 1. Otherwise, it is 0.
	u8 out_mode_y; // double:4, single:0
	u8 out_mode_c; // mode_0:0, mode_1:1, mode_2:2, mode_3:4
	u8 endian; //128-bit big endian:15, 128-bit LSB priority byte ordering:0
};


void sclr_set_base_addr(void *vi_base, void *vd0_base, void *vd1_base, void *vo_base);
void sclr_init_sys_top_addr(void);
void sclr_deinit_sys_top_addr(void);
void sclr_reg_force_up(u8 inst);
void sclr_top_set_cfg(u8 inst, bool sc_enable, bool fbd_enable);
void sclr_rt_set_cfg(u8 inst, union sclr_rt_cfg cfg);
union sclr_rt_cfg sclr_rt_get_cfg(u8 inst);
void sclr_top_reg_done(u8 inst);
u8 sclr_top_pg_late_get_bus(u8 inst);
void sclr_top_pg_late_clr(u8 inst);
void sclr_top_bld_set_cfg(struct sclr_bld_cfg *cfg);
void sclr_top_get_sb_default(struct sclr_top_sb_cfg *cfg);
void sclr_top_set_sb(struct sclr_top_sb_cfg *cfg);
void sclr_top_set_src_share(u8 inst, bool is_share);
void sclr_set_cfg(u8 inst, bool sc_bypass, bool gop_bypass,
		  bool cir_bypass, bool odma_bypass);
struct sclr_core_cfg *sclr_get_cfg(u8 inst);
void sclr_set_input_size(u8 inst, struct sclr_size src_rect, bool update);
void sclr_set_crop(u8 inst, struct sclr_rect crop_rect, bool is_update);
void sclr_set_output_size(u8 inst, struct sclr_size rect);
void sclr_set_scale_mir(u8 inst, bool enable);
void sclr_set_scale_phase(u8 inst, u32 h_ph, u32 v_ph);
void sclr_set_scale(u8 inst);
struct sclr_status sclr_get_status(u8 inst);
void sclr_read_2tap_nor(u8 inst, u16 *resize_hnor, u16 *resize_vnor);
void sclr_update_coef(u8 inst, enum sclr_algorithm coef);

void sclr_img_reg_shadow_sel(u8 inst, bool read_shadow);
void sclr_img_set_cfg(u8 inst, struct sclr_img_cfg *cfg);
struct sclr_img_cfg *sclr_img_get_cfg(u8 inst);
void sclr_vpss_sw_top_reset(u8 inst);
void sclr_img_reset(u8 inst);
void sclr_img_start(u8 inst);
void sclr_slave_ready(u8 inst);
void sclr_img_set_fmt(u8 inst, enum sclr_format fmt);
void sclr_img_set_mem(u8 inst, struct sclr_mem *mem, bool update);
void sclr_img_set_addr(u8 inst, u64 addr0, u64 addr1, u64 addr2);
void sclr_img_csc_en(u8 inst, bool enable);
void sclr_img_set_csc(u8 inst, struct sclr_csc_matrix *cfg);
union sclr_img_dbg_status sclr_img_get_dbg_status(u8 inst, bool clr);
void sclr_img_checksum_en(u8 inst, bool enable);
void sclr_img_get_checksum_status(u8 inst, struct sclr_img_checksum_status *status);
void sclr_img_dup2fancy_bypass(u8 inst, bool enable);

void sclr_oenc_set_cfg(struct sclr_oenc_cfg *oenc_cfg);
struct sclr_oenc_cfg *sclr_oenc_get_cfg(void);
void sclr_cover_set_cfg(u8 inst, u8 cover_w_inst, struct sclr_cover_cfg *cover_cfg, bool update);
void sclr_img_set_trig(u8 inst, enum sclr_img_trig_src trig_src);
void sclr_img_get_sb_default(struct sclr_img_in_sb_cfg *cfg);
void sclr_cir_set_cfg(u8 inst, struct sclr_cir_cfg *cfg);
void sclr_odma_set_cfg(u8 inst, struct sclr_odma_cfg *cfg);
struct sclr_odma_cfg *sclr_odma_get_cfg(u8 inst);
void sclr_odma_set_fmt(u8 inst, enum sclr_format fmt);
void sclr_odma_set_mem(u8 inst, struct sclr_mem *mem);
void sclr_odma_set_addr(u8 inst, u64 addr0, u64 addr1, u64 addr2);
union sclr_odma_dbg_status sclr_odma_get_dbg_status(u8 inst);
void sclr_set_out_mode(u8 inst, enum sclr_out_mode mode);
void sclr_set_quant(u8 inst, struct sclr_quant_formula *cfg);
void sclr_set_convert_to(u8 inst, struct sclr_convertto_formula *cfg);
void sclr_border_set_cfg(u8 inst, struct sclr_border_cfg *cfg);
void sclr_border_vpp_set_cfg(u8 inst, u8 border_idx, struct sclr_border_vpp_cfg *cfg, bool update);
struct sclr_border_vpp_cfg * sclr_border_vpp_get_cfg(u8 inst, u8 border_idx);
struct sclr_border_cfg *sclr_border_get_cfg(u8 inst);
void sclr_set_csc_ctrl(u8 inst, struct sclr_csc_cfg *cfg);
struct sclr_csc_cfg *sclr_get_csc_ctrl(u8 inst);
void sclr_set_csc(u8 inst, struct sclr_csc_matrix *cfg);
void sclr_get_csc(u8 inst, struct sclr_csc_matrix *cfg);
void sclr_core_set_cfg(u8 inst, struct sclr_core_cfg *cfg);
void sclr_core_checksum_en(u8 inst, bool enable);
void sclr_core_get_checksum_status(u8 inst, struct sclr_core_checksum_status *status);
void sclr_intr_ctrl(u8 inst, union sclr_intr intr_mask);
union sclr_intr sclr_get_intr_mask(u8 inst);
void sclr_set_intr_mask(u8 inst, union sclr_intr intr_mask);
void sclr_intr_clr(u8 inst, union sclr_intr intr_mask);
union sclr_intr sclr_intr_status(u8 inst);

void sclr_gop_set_cfg(u8 inst, u8 layer, struct sclr_gop_cfg *cfg, bool update);
struct sclr_gop_cfg *sclr_gop_get_cfg(u8 inst, u8 layer);
void sclr_gop_ow_set_cfg(u8 inst, u8 layer, u8 ow_inst, struct sclr_gop_ow_cfg *ow_cfg, bool update);
int sclr_gop_setup_256LUT(u8 inst, u8 layer, u16 length, u16 *data);
int sclr_gop_update_256LUT(u8 inst, u8 layer, u8 index, u16 data);
int sclr_gop_setup_16LUT(u8 inst, u8 layer, u8 length, u16 *data);
int sclr_gop_update_16LUT(u8 inst, u8 layer, u8 index, u16 data);
void sclr_gop_fb_set_cfg(u8 inst, u8 layer, u8 fb_inst, struct sclr_gop_fb_cfg *cfg);
u32 sclr_gop_fb_get_record(u8 inst, u8 layer, u8 fb_inst);

void sclr_pri_set_cfg(u8 inst, struct sclr_privacy_cfg *cfg);
void sclr_gop_odec_set_cfg_from_oenc(u8 inst, u8 layer, struct sclr_gop_odec_cfg *odec_cfg);

void sclr_ctrl_init(u8 inst, bool is_resume);
void sclr_get_2tap_scale(struct sclr_scale_2tap_cfg *cfg);
void sclr_ctrl_set_scale(u8 inst, struct sclr_scale_cfg *cfg);
int sclr_ctrl_set_input(u8 inst, enum sclr_input input,
			enum sclr_format fmt, enum sclr_csc csc);
int sclr_ctrl_set_output(u8 inst, struct sclr_csc_cfg *cfg,
			 enum sclr_format fmt);
void sclr_engine_cmdq(u8 inst, struct sclr_ctrl_cfg *cfgs, u8 cnt,
							u64 cmdq_phy_addr, void *cmdq_vir_addr,
							u32 cmdq_buf_size);

void sclr_clr_cmdq(u8 inst);
int sclr_set_fbd(u8 inst, struct sclr_fbd_cfg *cfg, bool is_update);
struct sclr_fbd_cfg *sclr_fbd_get_cfg(u8 inst);
struct sclr_csc_matrix *sclr_get_csc_mtrx(enum sclr_csc csc);

u8 sclr_tile_cal_size(u8 inst, u16 out_l_end);
u8 sclr_v_tile_cal_size(u8 inst, u16 out_l_end);
bool sclr_left_tile(u8 inst, u16 src_l_w);
bool sclr_right_tile(u8 inst, u16 src_offset);
bool sclr_down_tile(u8 inst, u16 src_offset, u8 is_right);
bool sclr_top_tile(u8 inst, u16 src_l_h, u8 is_right);

void sclr_dump_top_register(u8 inst);
void sclr_dump_img_in_register(int img_inst);
void sclr_dump_core_register(int inst);
void sclr_dump_odma_register(u8 inst);
void sclr_dump_register(u8 inst);
int vpss_online_multi(int w, int h, u64 *dst_addr);
int vpss_online_single(enum vpss_dev inst, int w, int h, u64 dst_addr);
int vpss_online_check_irq_handler(int timeout);
int vpss_dwa_online_multi(int w, int h, enum sclr_format fmt_in, u64 *dst_addr);
int vpss_dwa_online_single(enum vpss_dev inst, int w, int h, enum sclr_format fmt_in, u64 dst_addr);
int vpss_stitch_online_single(enum vpss_dev inst, int w, int h, enum sclr_format fmt_out, u64 src_addr);

#endif  //_SCL_H_

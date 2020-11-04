/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2017 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef _DE_RTMX_H_
#define _DE_RTMX_H_

#include <linux/types.h>
#include "../../include.h"
#include "de_top.h"

enum de_alpha_mode {
	DE_ALPHA_MODE_PIXEL_ALPHA   = 0x00000000,
	DE_ALPHA_MODE_GLOBAL_ALPHA  = 0x00000001,
	DE_ALPHA_MODE_PXL_GLB_ALPHA = 0x00000002,

	DE_ALPHA_MODE_PREMUL_SHIFT  = 0x8,
	DE_ALPHA_MODE_PREMUL_MASK   = 0x1 << DE_ALPHA_MODE_PREMUL_SHIFT,
	DE_ALPHA_MODE_PREMUL        = 0x01 << DE_ALPHA_MODE_PREMUL_SHIFT,
};

enum de_transform {
	DE_TRANSFORM_ROT_0         = 0x00,
	DE_TRANSFORM_ROT_90        = 0x01,
	DE_TRANSFORM_ROT_180       = 0x02,
	DE_TRANSFORM_ROT_270       = 0x03,
	DE_TRANSFORM_FLIP_H        = 0x04,
	DE_TRANSFORM_ROT_90_FLIP_H = 0x05,
	DE_TRANSFORM_FLIP_V        = 0x06,
	DE_TRANSFORM_ROT_90_FLIP_V = 0x07,
};

struct de_point {
	s32 x;
	s32 y;
};

struct de_size {
	u32 width;
	u32 height;
};

struct de_size64 {
	u64 width;
	u64 height;
};

struct de_rect {
	s32 left;
	s32 top;
	s32 right;
	s32 bottom;
};

struct de_rect_s {
	s32 left;
	s32 top;
	u32 width;
	u32 height;
};

struct de_rect_o {
	s32 x;
	s32 y;
	u32 w;
	u32 h;
};

/*
* low 32bit are frac
* high 32bit are fixed
*/
struct de_rect64_s {
	s64 left;
	s64 top;
	u64 width;
	u64 height;
};

struct de_scale_para {
	s32 hphase; /* initial phase of vsu/gsu in horizon */
	s32 vphase; /* initial phase of vsu/gsu in vertical */
	u32 hstep; /* scale step of vsu/gsu in horizon */
	u32 vstep; /* scale step of vsu/gsu in vertical */
};

enum de_layer_mode {
	DE_LAYER_MODE_BUF = 0,
	DE_LAYER_MODE_SOLID_COLOR = 1,
};

enum de_format_space {
	DE_FORMAT_SPACE_RGB = 0,
	DE_FORMAT_SPACE_YUV,
	DE_FORMAT_SPACE_IPT,
	DE_FORMAT_SPACE_GRAY,
};

enum de_3d_in_mode {
	DE_3D_SRC_NORMAL = 0x0,
	/* not 3d mode */
	DE_3D_SRC_MODE_TB = 0x1 << 0,
	/* top bottom */
	DE_3D_SRC_MODE_FP = 0x1 << 1,
	/* frame packing */
	DE_3D_SRC_MODE_SSF = 0x1 << 2,
	/* side by side full */
	DE_3D_SRC_MODE_SSH = 0x1 << 3,
	/* side by side half */
	DE_3D_SRC_MODE_LI = 0x1 << 4,
	/* line interleaved */
};

enum de_3d_out_mode {
	DE_3D_OUT_MODE_CI_1 = 0x5,
	/* column interleaved 1 */
	DE_3D_OUT_MODE_CI_2 = 0x6,
	/* column interleaved 2 */
	DE_3D_OUT_MODE_CI_3 = 0x7,
	/* column interleaved 3 */
	DE_3D_OUT_MODE_CI_4 = 0x8,
	/* column interleaved 4 */
	DE_3D_OUT_MODE_LIRGB = 0x9,
	/* line interleaved rgb */

	DE_3D_OUT_MODE_TB = 0x0,
	/* top bottom */
	DE_3D_OUT_MODE_FP = 0x1,
	/* frame packing */
	DE_3D_OUT_MODE_SSF = 0x2,
	/* side by side full */
	DE_3D_OUT_MODE_SSH = 0x3,
	/* side by side half */
	DE_3D_OUT_MODE_LI = 0x4,
	/* line interleaved */
	DE_3D_OUT_MODE_FA = 0xa,
	/* field alternative */
};

enum de_pixel_format {
	DE_FORMAT_ARGB_8888 = 0x00, /*MSB  A-R-G-B  LSB */
	DE_FORMAT_ABGR_8888 = 0x01,
	DE_FORMAT_RGBA_8888 = 0x02,
	DE_FORMAT_BGRA_8888 = 0x03,
	DE_FORMAT_XRGB_8888 = 0x04,
	DE_FORMAT_XBGR_8888 = 0x05,
	DE_FORMAT_RGBX_8888 = 0x06,
	DE_FORMAT_BGRX_8888 = 0x07,
	DE_FORMAT_RGB_888 = 0x08,
	DE_FORMAT_BGR_888 = 0x09,
	DE_FORMAT_RGB_565 = 0x0a,
	DE_FORMAT_BGR_565 = 0x0b,
	DE_FORMAT_ARGB_4444 = 0x0c,
	DE_FORMAT_ABGR_4444 = 0x0d,
	DE_FORMAT_RGBA_4444 = 0x0e,
	DE_FORMAT_BGRA_4444 = 0x0f,
	DE_FORMAT_ARGB_1555 = 0x10,
	DE_FORMAT_ABGR_1555 = 0x11,
	DE_FORMAT_RGBA_5551 = 0x12,
	DE_FORMAT_BGRA_5551 = 0x13,
	DE_FORMAT_ARGB_2101010 = 0x14,
	DE_FORMAT_ABGR_2101010 = 0x15,
	DE_FORMAT_RGBA_1010102 = 0x16,
	DE_FORMAT_BGRA_1010102 = 0x17,

	/*
	* SP: semi-planar, P:planar, I:interleaved
	* UVUV: U in the LSBs;     VUVU: V in the LSBs
	*/
	DE_FORMAT_YUV444_I_AYUV = 0x40,
	DE_FORMAT_YUV444_I_VUYA = 0x41,
	DE_FORMAT_YUV422_I_YVYU = 0x42,
	DE_FORMAT_YUV422_I_YUYV = 0x43,
	DE_FORMAT_YUV422_I_UYVY = 0x44,
	DE_FORMAT_YUV422_I_VYUY = 0x45,
	DE_FORMAT_YUV444_P = 0x46,
	DE_FORMAT_YUV422_P = 0x47,
	DE_FORMAT_YUV420_P = 0x48,
	DE_FORMAT_YUV411_P = 0x49,
	DE_FORMAT_YUV422_SP_UVUV = 0x4a,
	DE_FORMAT_YUV422_SP_VUVU = 0x4b,
	DE_FORMAT_YUV420_SP_UVUV = 0x4c,
	DE_FORMAT_YUV420_SP_VUVU = 0x4d,
	DE_FORMAT_YUV411_SP_UVUV = 0x4e,
	DE_FORMAT_YUV411_SP_VUVU = 0x4f,
	DE_FORMAT_8BIT_GRAY                    = 0x50,
	DE_FORMAT_YUV444_I_AYUV_10BIT          = 0x51,
	DE_FORMAT_YUV444_I_VUYA_10BIT          = 0x52,
	DE_FORMAT_YUV422_I_YVYU_10BIT          = 0x53,
	DE_FORMAT_YUV422_I_YUYV_10BIT          = 0x54,
	DE_FORMAT_YUV422_I_UYVY_10BIT          = 0x55,
	DE_FORMAT_YUV422_I_VYUY_10BIT          = 0x56,
	DE_FORMAT_YUV444_P_10BIT               = 0x57,
	DE_FORMAT_YUV422_P_10BIT               = 0x58,
	DE_FORMAT_YUV420_P_10BIT               = 0x59,
	DE_FORMAT_YUV411_P_10BIT               = 0x5a,
	DE_FORMAT_YUV422_SP_UVUV_10BIT         = 0x5b,
	DE_FORMAT_YUV422_SP_VUVU_10BIT         = 0x5c,
	DE_FORMAT_YUV420_SP_UVUV_10BIT         = 0x5d,
	DE_FORMAT_YUV420_SP_VUVU_10BIT         = 0x5e,
	DE_FORMAT_YUV411_SP_UVUV_10BIT         = 0x5f,
	DE_FORMAT_YUV411_SP_VUVU_10BIT         = 0x60,
};

enum de_yuv_sampling {
	DE_YUV444 = 0,
	DE_YUV422,
	DE_YUV420,
	DE_YUV411,
};

enum de_eotf {
	DE_EOTF_RESERVED = 0x000,
	DE_EOTF_BT709 = 0x001,
	DE_EOTF_UNDEF = 0x002,
	DE_EOTF_GAMMA22 = 0x004, /* SDR */
	DE_EOTF_GAMMA28 = 0x005,
	DE_EOTF_BT601 = 0x006,
	DE_EOTF_SMPTE240M = 0x007,
	DE_EOTF_LINEAR = 0x008,
	DE_EOTF_LOG100 = 0x009,
	DE_EOTF_LOG100S10 = 0x00a,
	DE_EOTF_IEC61966_2_4 = 0x00b,
	DE_EOTF_BT1361 = 0x00c,
	DE_EOTF_IEC61966_2_1 = 0X00d,
	DE_EOTF_BT2020_0 = 0x00e,
	DE_EOTF_BT2020_1 = 0x00f,
	DE_EOTF_SMPTE2084 = 0x010, /* HDR10 */
	DE_EOTF_SMPTE428_1 = 0x011,
	DE_EOTF_ARIB_STD_B67 = 0x012, /* HLG */
};

enum de_color_space_u {
	DE_UNDEF = 0x00,
	DE_UNDEF_F = 0x01,
	DE_GBR = 0x100,
	DE_BT709 = 0x101,
	DE_FCC = 0x102,
	DE_BT470BG = 0x103,
	DE_BT601 = 0x104,
	DE_SMPTE240M = 0x105,
	DE_YCGCO = 0x106,
	DE_BT2020NC = 0x107,
	DE_BT2020C = 0x108,
	DE_GBR_F = 0x200,
	DE_BT709_F = 0x201,
	DE_FCC_F = 0x202,
	DE_BT470BG_F = 0x203,
	DE_BT601_F = 0x204,
	DE_SMPTE240M_F = 0x205,
	DE_YCGCO_F = 0x206,
	DE_BT2020NC_F = 0x207,
	DE_BT2020C_F = 0x208,
	DE_RESERVED = 0x300,
	DE_RESERVED_F = 0x301,
};

enum de_color_space {
	DE_COLOR_SPACE_GBR = 0,
	DE_COLOR_SPACE_BT709 = 1,
	DE_COLOR_SPACE_FCC = 2,
	DE_COLOR_SPACE_BT470BG = 3,
	DE_COLOR_SPACE_BT601 = 4,
	DE_COLOR_SPACE_SMPTE240M = 5,
	DE_COLOR_SPACE_YCGCO = 6,
	DE_COLOR_SPACE_BT2020NC = 7,
	DE_COLOR_SPACE_BT2020C = 8,
};

enum de_color_range {
	DE_COLOR_RANGE_DEFAULT = 0, /*default*/
	DE_COLOR_RANGE_0_255  = 1, /*full*/
	DE_COLOR_RANGE_16_235 = 2, /*limited*/
};

enum de_data_bits {
	DE_DATA_8BITS  = 0,
	DE_DATA_10BITS = 1,
	DE_DATA_12BITS = 2,
	DE_DATA_16BITS = 3,
};

struct de_output_info {
	u8 tcon_en;
	u8 tcon_id;
	u32 scn_width;
	u32 scn_height;
	u64 fps;
	u32 scan_mode; /* 0:progress. 1:interlace */
	enum de_format_space px_fmt_space;
	enum de_yuv_sampling yuv_sampling;
	enum de_eotf eotf;
	enum de_color_space color_space;
	enum de_color_range color_range;
	enum de_data_bits data_bits;
	bool cvbs_direct_show;
};

struct de_chn_info {
	u8 enable;
	u8 fbd_en;
	u8 atw_en;
	u8 scale_en;
	u8 min_zorder;
	u8 glb_alpha;
	enum de_alpha_mode alpha_mode;
	u8 lay_premul[MAX_LAYER_NUM_PER_CHN];
	u32 flag;
	struct de_rect_s lay_win[MAX_LAYER_NUM_PER_CHN];
	struct de_rect_s ovl_win;
	struct de_rect_s ovl_out_win;
	struct de_rect_s c_win; /* for chroma */
	struct de_rect_s scn_win;
	struct de_scale_para ovl_ypara;
	struct de_scale_para ovl_cpara; /* for chroma */
	enum de_pixel_format px_fmt;
	enum de_format_space px_fmt_space;
	enum de_yuv_sampling yuv_sampling;
	u32 planar_num;
	enum de_eotf eotf;
	enum de_color_space color_space;
	enum de_color_range color_range;
	void *cdc_hdl;
	u8 snr_en;
};

struct de_port_layer_info {
	u8 enable;
	u32 flag;
	enum de_format_space px_fmt_space;
	enum de_yuv_sampling yuv_sampling;
	enum de_eotf eotf;
	enum de_color_space color_space;
	enum de_color_range color_range;
};

union rcq_hd_dw0 {
	u32 dwval;
	struct {
		u32 len:24;
		u32 high_addr:8;
	} bits;
};

union rcq_hd_dirty {
	u32 dwval;
	struct {
		u32 dirty:1;
		u32 res0:31;
	} bits;
};

struct de_rcq_head {
	u32 low_addr; /* 32 bytes align */
	union rcq_hd_dw0 dw0;
	union rcq_hd_dirty dirty;
	u32 reg_offset; /* offset_addr based on de_reg_base */
};

/*
* @phy_addr: must be 32 bytes align, can not be accessed by cpu.
* @vir_addr: for cpu access.
* @size: unit: byte. must be 2 bytes align.
* @reg_addr: reg base addr of this block.
* @dirty: this block need be updated to hw reg if @dirty is true.
* @rcq_hd: pointer to rcq head of this dma_reg block at rcq mode.
*/
struct de_reg_block {
	u8 __iomem *phy_addr;
	u8 *vir_addr;
	u32 size;
	u8 __iomem *reg_addr;
	u32 dirty;
	struct de_rcq_head *rcq_hd;
};

struct de_reg_mem_info {
	u8 __iomem *phy_addr; /* it is non-null at rcq mode */
	u8 *vir_addr;
	u32 size;
};

struct de_rcq_mem_info {
	u8 __iomem *phy_addr;
	struct de_rcq_head *vir_addr;
	struct de_reg_block **reg_blk;
	u32 alloc_num;
	u32 cur_num;
};

struct de_rtmx_context {
	u8 *__iomem de_reg_base;
	struct de_rcq_mem_info rcq_info;
	u64 clk_rate_hz;
	u32 bg_color;
	u32 be_blank;
	struct de_chn_info *chn_info;
	u32 chn_num;
	u32 min_z_chn_id;
	struct de_output_info output;
};


struct de_rtmx_context *de_rtmx_get_context(u32 disp);
s32 de_rtmx_init(struct disp_bsp_init_para *para);
s32 de_rtmx_exit(void);
s32 de_rtmx_start(u32 disp);
s32 de_rtmx_stop(u32 disp);
s32 de_rtmx_update_reg_ahb(u32 disp);
s32 de_rtmx_set_rcq_update(u32 disp, u32 en);
s32 de_rtmx_set_all_rcq_head_dirty(u32 disp, u32 dirty);
s32 de_rtmx_mgr_apply(u32 disp, struct disp_manager_data *data);
s32 de_rtmx_layer_apply(u32 disp,
	struct disp_layer_config_data *const data, u32 layer_num);
void de_rtmx_flush_layer_address(u32 disp, u32 chn, u32 layer_id);


#endif /* #ifndef _DE_RTMX_H_ */

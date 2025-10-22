/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_BASE_H_
#define _DE_BASE_H_

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <drm/drm_print.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_rect.h>

#define SETMASK(width, shift)   (((width)?((-1U) >> (32-(width))):0)  << (shift))
#define CLRMASK(width, shift)   (~(SETMASK(width, shift)))
#define GET_BITS(shift, width, reg)     \
	(((reg) & SETMASK(width, shift)) >> (shift))
#define SET_BITS(shift, width, reg, val) \
	(((reg) & CLRMASK(width, shift)) | ((val) << (shift)))

extern short g_gamma_funcs[17][1024];

enum de_format_space {
	DE_FORMAT_SPACE_RGB = 0,
	DE_FORMAT_SPACE_YUV,
	DE_FORMAT_SPACE_IPT,
	DE_FORMAT_SPACE_GRAY,
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

enum de_yuv_sampling {
	DE_YUV444 = 0,
	DE_YUV422,
	DE_YUV420,
	DE_YUV411,
};

enum de_data_bits {
	DE_DATA_8BITS  = 0,
	DE_DATA_10BITS = 1,
	DE_DATA_12BITS = 2,
	DE_DATA_16BITS = 3,
};

enum enhance_init_state {
	/* module only alloc mem*/
	ENHANCE_INVALID      = 0x00000000,
	/* module has inited pri para */
	ENHANCE_INITED       = 0x00000001,
	/* module is turned on by tigerlcd*/
	ENHANCE_TIGERLCD_ON  = 0x00000002,
	/* module is turned off by tigerlcd*/
	ENHANCE_TIGERLCD_OFF = 0x00000003,
};

typedef struct _demo_win_percent {
	u8 hor_start;
	u8 hor_end;
	u8 ver_start;
	u8 ver_end;
	u8 demo_en;
} win_percent_t;

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

enum de_update_mode {
	AHB_MODE,
	RCQ_MODE,
	DOUBLE_BUFFER_MODE,
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

struct de_reg_mem_info {
	u8 __iomem *phy_addr; /* it is non-null at rcq mode */
	u8 *vir_addr;
	u32 size;
};

struct de_rcq_head {
	u32 low_addr; /* 32 bytes align */
	union rcq_hd_dw0 dw0;
	union rcq_hd_dirty dirty;
	u32 reg_offset; /* offset_addr based on de_reg_base */
};

struct de_reg_block {
	u8 __iomem *phy_addr;
	u8 *vir_addr;
	u32 size;
	u8 __iomem *reg_addr;
	u32 dirty;
	struct de_rcq_head *rcq_hd;
};

struct module_create_info {
	unsigned int de_version;
	struct sunxi_display_engine *de;
	unsigned int id;
	unsigned int extra_id;
	unsigned int reg_offset;
	enum de_update_mode update_mode;
	void __iomem *de_reg_base;
	void *extra;
};


int drm_to_de_format(uint32_t format);
void *sunxi_de_reg_buffer_alloc(struct sunxi_display_engine *de, u32 size, dma_addr_t *phy_addr, u32 rcq_used);
void *sunxi_de_dma_alloc_coherent(struct sunxi_display_engine *de, u32 size, dma_addr_t *phy_addr);
void sunxi_de_dma_free_coherent(struct sunxi_display_engine *de, u32 size, dma_addr_t phy_addr, void *virt_addr);

#endif

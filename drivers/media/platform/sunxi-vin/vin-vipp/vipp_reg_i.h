/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-vipp/vipp_reg_i.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 ******************************************************************************
 *
 * vipp_reg_i.h
 *
 * VIPP - vipp_reg_i.h module
 *
 * Copyright (c) 2016 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version         Author         Date        Description
 *
 *   1.0         Zhao Wei    2016/07/19      First Version
 *
 ******************************************************************************
 */
#ifndef __VIPP__REG__I__H__
#define __VIPP__REG__I__H__

/*
 * Detail information of registers
 */

#define VIPP_TOP_EN_REG_OFF		0X000
#define VIPP_CLK_GATING_EN		0
#define VIPP_CLK_GATING_EN_MASK		(0X1 << VIPP_CLK_GATING_EN)

#define VIPP_EN_REG_OFF			0X004
#define VIPP_EN				0
#define VIPP_EN_MASK			(0X1 << VIPP_EN)
#define VIPP_VER_EN			3
#define VIPP_VER_EN_MASK		(0X1 << VIPP_VER_EN)

#define VIPP_VER_REG_OFF		0X008
#define VIPP_SMALL_VER			0
#define VIPP_SMALL_VER_MASK		(0XFFF << VIPP_SMALL_VER)
#define VIPP_BIG_VER			12
#define VIPP_BIG_VER_MASK		(0XFFF << VIPP_BIG_VER)

#define VIPP_FEATURE_REG_OFF		0X00C
#define VIPP_OSD_EXIST			0
#define VIPP_OSD_EXIST_MASK		(0X1 << VIPP_OSD_EXIST)
#define VIPP_FBC_EXIST			1
#define VIPP_FBC_EXIST_MASK		(0X1 << VIPP_FBC_EXIST)

#define VIPP_CTRL_REG_OFF		0X014
#define VIPP_OSD_OV_UPDATE		0
#define VIPP_OSD_OV_UPDATE_MASK		(0X1 << VIPP_OSD_OV_UPDATE)
#define VIPP_OSD_CV_UPDATE		1
#define VIPP_OSD_CV_UPDATE_MASK		(0X1 << VIPP_OSD_CV_UPDATE)
#define VIPP_PARA_READY			2
#define VIPP_PARA_READY_MASK		(0X1 << VIPP_PARA_READY)

#define VIPP_OSD_LOAD_ADDR_REG_OFF	0X018
#define VIPP_OSD_STAT_ADDR_REG_OFF	0X01C
#define VIPP_OSD_BM_ADDR_REG_OFF	0X020
#define VIPP_REG_LOAD_ADDR_REG_OFF	0X024

#define VIPP_STA_REG_OFF		0X030
#define VIPP_REG_LOAD_PD		0
#define VIPP_REG_LOAD_PD_MASK		(0X01 << VIPP_REG_LOAD_PD)
#define VIPP_BM_ERROR_PD		2
#define VIPP_BM_ERROR_PD_MASK		(0X01 << VIPP_REG_LOAD_PD)

#define VIPP_MODULE_EN_REG_OFF		0X040
#define VIPP_SC_CFG_REG_OFF		0X044
#define VIPP_SC_SIZE_REG_OFF		0X048
#define VIPP_MODE_REG_OFF		0X04C
#define VIPP_OSD_CFG_REG_OFF		0X050
#define VIPP_OSD_GAIN0_REG_OFF		0X054
#define VIPP_OSD_GAIN1_REG_OFF		0X058
#define VIPP_OSD_GAIN2_REG_OFF		0X05C
#define VIPP_OSD_GAIN3_REG_OFF		0X060
#define VIPP_OSD_GAIN4_REG_OFF		0X064
#define VIPP_OSD_OFFSET_REG_OFF		0X068
#define VIPP_CROP_START_REG_OFF		0X070
#define VIPP_CROP_SIZE_REG_OFF		0X074

typedef union {
	unsigned int dwval;
	struct {
		unsigned int sc_en:1;
		unsigned int osd_en:1;
		unsigned int chroma_ds_en:1;
		unsigned int res0:29;
	} bits;
} VIPP_MODULE_EN_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int sc_out_fmt:1;
		unsigned int res0:3;
		unsigned int sc_xratio:12;
		unsigned int sc_yratio:12;
		unsigned int sc_weight_shift:4;
	} bits;
} VIPP_SCALER_CFG_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int sc_width:13;
		unsigned int res0:3;
		unsigned int sc_height:13;
		unsigned int res1:3;
	} bits;
} VIPP_SCALER_OUTPUT_SIZE_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int vipp_out_fmt:1;
		unsigned int vipp_in_fmt:1;
		unsigned int res0:30;
	} bits;
} VIPP_OUTPUT_FMT_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int osd_ov_en:1;
		unsigned int osd_cv_en:1;
		unsigned int osd_argb_mode:2;
		unsigned int osd_stat_en:1;
		unsigned int res0:3;
		unsigned int osd_ov_num:6;
		unsigned int res1:2;
		unsigned int osd_cv_num:3;
		unsigned int res2:13;
	} bits;
} VIPP_OSD_CFG_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int jc0:11;
		unsigned int res0:5;
		unsigned int jc1:11;
		unsigned int res1:5;
	} bits;
} VIPP_OSD_RGB2YUV_GAIN0_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int jc2:11;
		unsigned int res0:5;
		unsigned int jc3:11;
		unsigned int res1:5;
	} bits;
} VIPP_OSD_RGB2YUV_GAIN1_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int jc4:11;
		unsigned int res0:5;
		unsigned int jc5:11;
		unsigned int res1:5;
	} bits;
} VIPP_OSD_RGB2YUV_GAIN2_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int jc6:11;
		unsigned int res0:5;
		unsigned int jc7:11;
		unsigned int res1:5;
	} bits;
} VIPP_OSD_RGB2YUV_GAIN3_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int jc8:11;
		unsigned int res0:21;
	} bits;
} VIPP_OSD_RGB2YUV_GAIN4_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int jc9:9;
		unsigned int jc10:9;
		unsigned int jc11:9;
		unsigned int res0:5;
	} bits;
} VIPP_OSD_RGB2YUV_OFFSET_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int crop_hor_st:13;
		unsigned int res0:3;
		unsigned int crop_ver_st:13;
		unsigned int res1:5;
	} bits;
} VIPP_CROP_START_POSITION_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int crop_width:13;
		unsigned int res0:3;
		unsigned int crop_height:13;
		unsigned int res1:5;
	} bits;
} VIPP_CROP_SIZE_REG_t;

typedef union {
	unsigned long long ddwval;
	struct {
		unsigned long long h_start:13;
		unsigned long long h_end:13;
		unsigned long long res0:6;
		unsigned long long v_start:13;
		unsigned long long v_end:13;
		unsigned long long alpha:5;
		unsigned long long inverse_en:1;
	} bits;
} VIPP_OSD_OVERLAY_CFG_REG_t;

typedef union {
	unsigned long long ddwval;
	struct {
		unsigned long long h_start:13;
		unsigned long long h_end:13;
		unsigned long long res0:6;
		unsigned long long v_start:13;
		unsigned long long v_end:13;
		unsigned long long res1:6;
	} bits;
} VIPP_OSD_COVER_CFG_REG_t;

typedef union {
	unsigned long long ddwval;
	struct {
		unsigned long long y:8;
		unsigned long long u:8;
		unsigned long long v:8;
		unsigned long long res0:40;
	} bits;
} VIPP_OSD_COVER_DATA_REG_t;

#endif /*__VIPP__REG__I__H__*/

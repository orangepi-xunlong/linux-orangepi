/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
 *
 * Authors:  Zheng ZeQun <zequnzheng@allwinnertech.com>
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

#ifndef __VIPP200__REG__I__H__
#define __VIPP200__REG__I__H__

/*
 * Detail information of top registers
 */
#define VIPP_TOP_EN_REG_OFF		0X000
#define VIPP_CLK_GATING_EN		0
#define VIPP_CLK_GATING_EN_MASK		(0X1 << VIPP_CLK_GATING_EN)
#define VIPP_CAP_EN			1
#define VIPP_CAP_EN_MASK		(0X1 << VIPP_CAP_EN)
#define VIPP_WORK_MODE			4
#define VIPP_WORK_MODE_MASK		(0X1 << VIPP_WORK_MODE)
#define VIPP_MODULE_CLK_BACK_DOOR	6
#define VIPP_MODULE_CLK_BACK_DOOR_MASK	(0X1 << VIPP_MODULE_CLK_BACK_DOOR)
#define VIPP_VER_EN			7
#define VIPP_VER_EN_MASK		(0X1 << VIPP_VER_EN)

#define VIPP_VER_REG_OFF		0X008
#define VIPP_SMALL_VER			0
#define VIPP_SMALL_VER_MASK		(0XFFF << VIPP_SMALL_VER)
#define VIPP_BIG_VER			12
#define VIPP_BIG_VER_MASK		(0XFFF << VIPP_BIG_VER)

#define VIPP_FEATURE_REG_OFF		0X00C
#define VIPP_YUV422TO420		1
#define VIPP_YUV422TO420_MASK		(0X1 << VIPP_YUV422TO420)
#define VIPP_ORL_EXIST			2
#define VIPP_ORL_EXIST_MASK		(0X1 << VIPP_ORL_EXIST)

#define VIPP_INT_BYPASS_REG_OFF		0X020
#define VIPP_CHN0_REG_LOAD_EN		8
#define VIPP_CHN0_REG_LOAD_EN_MASK	(0X1 << VIPP_CHN0_REG_LOAD_EN)
#define VIPP_CHN1_REG_LOAD_EN		14
#define VIPP_CHN1_REG_LOAD_EN_MASK	(0X1 << VIPP_CHN1_REG_LOAD_EN)
#define VIPP_CHN2_REG_LOAD_EN		20
#define VIPP_CHN2_REG_LOAD_EN_MASK	(0X1 << VIPP_CHN2_REG_LOAD_EN)
#define VIPP_CHN3_REG_LOAD_EN		26
#define VIPP_CHN3_REG_LOAD_EN_MASK	(0X1 << VIPP_CHN3_REG_LOAD_EN)

#define VIPP_INT_BYPASS1_REG_OFF		0X024

#define VIPP_INT_STATUS_REG_OFF		0X030
#define VIPP_ID_LOST_PD			0
#define VIPP_ID_LOST_PD_MASK		(0X1 << VIPP_ID_LOST_PD)
#define VIPP_AHB_MBUS_W_PD		1
#define VIPP_AHB_MBUS_W_PD_MASK		(0X1 << VIPP_AHB_MBUS_W_PD)
#define VIPP_CHN0_REG_LOAD_PD		8
#define VIPP_CHN0_REG_LOAD_PD_MASK	(0X1 << VIPP_CHN0_REG_LOAD_PD)
#define VIPP_CHN0_FRM_LOST_PD		9
#define VIPP_CHN0_FRM_LOST_PD_MASK	(0X1 << VIPP_CHN0_FRM_LOST_PD)
#define VIPP_CHN0_HB_SHORT_PD		10
#define VIPP_CHN0_HB_SHORT_PD_MASK	(0X1 << VIPP_CHN0_HB_SHORT_PD)
#define VIPP_CHN0_PAPA_NOTREADY_PD	11
#define VIPP_CHN0_PAPA_NOTREADY_PD_MASK	(0X1 << VIPP_CHN0_PAPA_NOTREADY_PD)
#define VIPP_CHN0_HSHORT_PD			12
#define VIPP_CHN0_HSHORT_PD_MASK	(0X1 << VIPP_CHN0_HSHORT_PD)
#define VIPP_CHN0_VSHORT_PD			13
#define VIPP_CHN0_VSHORT_PD_MASK	(0X1 << VIPP_CHN0_VSHORT_PD)

#define VIPP_CHN1_REG_LOAD_PD		14
#define VIPP_CHN1_REG_LOAD_PD_MASK	(0X1 << VIPP_CHN1_REG_LOAD_PD)
#define VIPP_CHN1_FRM_LOST_PD		15
#define VIPP_CHN1_FRM_LOST_PD_MASK	(0X1 << VIPP_CHN1_FRM_LOST_PD)
#define VIPP_CHN1_HB_SHORT_PD		16
#define VIPP_CHN1_HB_SHORT_PD_MASK	(0X1 << VIPP_CHN1_HB_SHORT_PD)
#define VIPP_CHN1_PAPA_NOTREADY_PD	17
#define VIPP_CHN1_PAPA_NOTREADY_PD_MASK	(0X1 << VIPP_CHN1_PAPA_NOTREADY_PD)
#define VIPP_CHN1_HSHORT_PD			18
#define VIPP_CHN1_HSHORT_PD_MASK	(0X1 << VIPP_CHN1_HSHORT_PD)
#define VIPP_CHN1_VSHORT_PD			19
#define VIPP_CHN1_VSHORT_PD_MASK	(0X1 << VIPP_CHN1_VSHORT_PD)

#define VIPP_CHN2_REG_LOAD_PD		20
#define VIPP_CHN2_REG_LOAD_PD_MASK	(0X1 << VIPP_CHN2_REG_LOAD_PD)
#define VIPP_CHN2_FRM_LOST_PD		21
#define VIPP_CHN2_FRM_LOST_PD_MASK	(0X1 << VIPP_CHN2_FRM_LOST_PD)
#define VIPP_CHN2_HB_SHORT_PD		22
#define VIPP_CHN2_HB_SHORT_PD_MASK	(0X1 << VIPP_CHN2_HB_SHORT_PD)
#define VIPP_CHN2_PAPA_NOTREADY_PD	23
#define VIPP_CHN2_PAPA_NOTREADY_PD_MASK	(0X1 << VIPP_CHN2_PAPA_NOTREADY_PD)
#define VIPP_CHN2_HSHORT_PD			24
#define VIPP_CHN2_HSHORT_PD_MASK	(0X1 << VIPP_CHN2_HSHORT_PD)
#define VIPP_CHN2_VSHORT_PD			25
#define VIPP_CHN2_VSHORT_PD_MASK	(0X1 << VIPP_CHN2_VSHORT_PD)

#define VIPP_CHN3_REG_LOAD_PD		26
#define VIPP_CHN3_REG_LOAD_PD_MASK	(0X1 << VIPP_CHN3_REG_LOAD_PD)
#define VIPP_CHN3_FRM_LOST_PD		27
#define VIPP_CHN3_FRM_LOST_PD_MASK	(0X1 << VIPP_CHN3_FRM_LOST_PD)
#define VIPP_CHN3_HB_SHORT_PD		28
#define VIPP_CHN3_HB_SHORT_PD_MASK	(0X1 << VIPP_CHN3_HB_SHORT_PD)
#define VIPP_CHN3_PAPA_NOTREADY_PD	29
#define VIPP_CHN3_PAPA_NOTREADY_PD_MASK	(0X1 << VIPP_CHN3_PAPA_NOTREADY_PD)
#define VIPP_CHN3_HSHORT_PD			30
#define VIPP_CHN3_HSHORT_PD_MASK	(0X1 << VIPP_CHN3_HSHORT_PD)
#define VIPP_CHN3_VSHORT_PD			31
#define VIPP_CHN3_VSHORT_PD_MASK	(0X1 << VIPP_CHN3_VSHORT_PD)

#if !defined CONFIG_ARCH_SUN55IW3
#define VIPP_INT_STATUS1_REG_OFF		0X034
#define VIPP_CHN0_FRM_DONE_PD			1
#define VIPP_CHN0_FRM_DONE_PD_MASK	(0X1 << VIPP_CHN0_FRM_DONE_PD)
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
#define VIPP_RETURN_INF_REG_OFF		0X034
#else
#define VIPP_RETURN_INF_REG_OFF		0X038
#endif
#define VIPP_AHB_MBUS_W_ADDR		0
#define VIPP_AHB_MBUS_W_ADDR_MASK	(0X1FFF << VIPP_AHB_MBUS_W_ADDR)
#define VIPP_SUB_ID			16
#define VIPP_SUB_ID_MASK		(0X3 << VIPP_SUB_ID)

/*
 * Detail information of ch registers
 */
#define VIPP_CH_OFFSET			0X40
#define VIPP_CH_AMONG_OFFSET		0X40

#define VIPP_CH_CTRL_REG_OFF		0X0
#define VIPP_CHN_CAP_EN			0
#define VIPP_CHN_CAP_EN_MASK		(0X1 << VIPP_CHN_CAP_EN)
#define VIPP_PARA_READY			2
#define VIPP_PARA_READY_MASK		(0X1 << VIPP_PARA_READY)
#define VIPP_BYPASS_MODE		4
#define VIPP_BYPASS_MODE_MASK		(0X1 << VIPP_BYPASS_MODE)

#define VIPP_REG_LOAD_ADDR_REG_OFF	0X010

/*
 * Detail information of load registers
 */
#define VIPP_LOAD_OFFSET		0X200

#define VIPP_MODULE_EN_REG_OFF		0X000
#define VIPP_SC_CFG_REG_OFF		0X004
#define VIPP_SC_SIZE_REG_OFF		0X008
#define VIPP_MODE_REG_OFF		0X020
#define VIPP_CROP_START_REG_OFF		0X030
#define VIPP_CROP_SIZE_REG_OFF		0X034
#define VIPP_CGC_GAIN_CTRL_REG_OFF	0X038
#define VIPP_CGC_CLIP_CTRL_REG		0X03C
#define VIPP_ORL_CONTROL_REG_OFF	0X040
#define VIPP_ORL_START0_REG_OFF  	0X050
#define VIPP_ORL_END0_REG_OFF		0X090
#define VIPP_ORL_YUV0_REG_OFF		0X0d0

#define VIPP_YUV2RGB_CFG0_REG_OFF		0X110
#define VIPP_YUV2RGB_CFG1_REG_OFF		0X114
#define VIPP_YUV2RGB_CFG2_REG_OFF		0X118
#define VIPP_YUV2RGB_CFG3_REG_OFF		0X11c
#define VIPP_YUV2RGB_CFG4_REG_OFF		0X120
#define VIPP_YUV2RGB_CFG5_REG_OFF		0X124

/*
 * Detail information of load params struct
 */
typedef union {
	unsigned int dwval;
	struct {
		unsigned int sc_en:1;
		unsigned int res0:1;
		unsigned int chroma_ds_en:1;
		unsigned int yuv2rgb_en:1;
		unsigned int cgc_f2l_en:1;
		unsigned int res1:27;
	} bits;
} VIPP_MODULE_EN_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int sc_xratio:13;
		unsigned int res0:1;
		unsigned int sc_yratio:13;
		unsigned int sc_weight_shift:5;
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

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
typedef union {
	unsigned int dwval;
	struct {
		unsigned int vipp_out_fmt:1;
		unsigned int vipp_in_fmt:1;
		unsigned int sc_out_fmt:1;
		unsigned int res0:29;
	} bits;
} VIPP_OUTPUT_FMT_REG_t;
#else
typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:4;
		unsigned int vipp_out_fmt:2;
		unsigned int vipp_in_fmt:1;
		unsigned int sc_out_fmt:1;
		unsigned int res1:24;
	} bits;
} VIPP_OUTPUT_FMT_REG_t;
#endif

typedef union {
	unsigned int dwval;
	struct {
		unsigned int crop_hor_st:13;
		unsigned int res0:3;
		unsigned int crop_ver_st:13;
		unsigned int res1:3;
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
	unsigned int dwval;
	struct {
		unsigned int cgc_f2l_gain_y:8;
		unsigned int cgc_f2l_gain_uv:8;
		unsigned int cgc_f2l_offset_y:8;
		unsigned int cgc_f2l_offset_uv:8;
	} bits;
} VIPP_CGC_GAIN_CTRL_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int cgc_f2l_y_min:8;
		unsigned int cgc_f2l_y_max:8;
		unsigned int cgc_f2l_uv_min:8;
		unsigned int cgc_f2l_uv_max:8;
	} bits;
} VIPP_CGC_CLIP_CTRL_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int orl_num:5;
		unsigned int res0:3;
		unsigned int orl_width:3;
		unsigned int res1:21;
	} bits;
} VIPP_ORL_CONTROL_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int orl_xs:13;
		unsigned int res0:3;
		unsigned int orl_ys:13;
		unsigned int res1:3;
	} bits;
} VIPP_ORL_START_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int orl_xe:13;
		unsigned int res0:3;
		unsigned int orl_ye:13;
		unsigned int res1:3;
	} bits;
} VIPP_ORL_END_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int orl_y:8;
		unsigned int orl_u:8;
		unsigned int orl_v:8;
		unsigned int res0:8;
	} bits;
} VIPP_ORL_YUV_REG_t;

typedef union {
	unsigned int dwval;
} VIPP_YUV2RGB_CFG_REG_t;

#endif /*__VIPP200__REG__I__H__*/

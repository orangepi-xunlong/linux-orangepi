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

#include <linux/kernel.h>
#include "vipp200_reg_i.h"
#include "vipp200_reg.h"

#include "../../utility/vin_io.h"
#include "../../platform/platform_cfg.h"

/*#define VIPP_SCALER_DIRECTLY_WRITE_REG*/
#define VIPP_ADDR_BIT_R_SHIFT 2

#define addr_base_offset 0x4

#if IS_ENABLED(CONFIG_ARCH_SUN60IW1)
int vipp_virtual_find_ch[24] = {
	0, 1, 2, 3, 0, 1, 2, 3, 0, 0, 0, 0, 0, 1, 2, 3, 0, 1, 2, 3, 0, 0, 0, 0,
};
int vipp_virtual_find_logic[24] = {
	0, 0, 0, 0, 4, 4, 4, 4, 8, 9, 10, 11, 12, 12, 12, 12, 16, 16, 16, 16, 20, 21, 22, 23,
};
#else
int vipp_virtual_find_ch[18] = {
	0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 0,
};
int vipp_virtual_find_logic[18] = {
	0, 0, 0, 0, 4, 4, 4, 4, 8, 8, 8, 8, 12, 12, 12, 12, 16, 17,
};
int vipp_virtual_find_sel[18] = {
	0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 5,
};
#endif

volatile void __iomem *vipp_base[VIN_MAX_SCALER];

struct vipp_reg {
	VIPP_MODULE_EN_REG_t *vipp_module_en;
	VIPP_SCALER_CFG_REG_t *vipp_scaler_cfg;
	VIPP_SCALER_OUTPUT_SIZE_REG_t *vipp_scaler_output_size;
	VIPP_OUTPUT_FMT_REG_t *vipp_output_fmt;
	VIPP_CROP_START_POSITION_REG_t *vipp_crop_start;
	VIPP_CROP_SIZE_REG_t *vipp_crop_size;
	VIPP_CGC_GAIN_CTRL_REG_t *vipp_cgc_gain_ctrl;
	VIPP_CGC_CLIP_CTRL_REG_t *vipp_cgc_clip_ctrl;
	VIPP_ORL_CONTROL_REG_t *vipp_orl_control;
	VIPP_ORL_START_REG_t *vipp_orl_start;
	VIPP_ORL_END_REG_t *vipp_orl_end;
	VIPP_ORL_YUV_REG_t *vipp_orl_yuv;

	VIPP_YUV2RGB_CFG_REG_t *vipp_yuv2rgb_cfg0;
	VIPP_YUV2RGB_CFG_REG_t *vipp_yuv2rgb_cfg1;
	VIPP_YUV2RGB_CFG_REG_t *vipp_yuv2rgb_cfg2;
	VIPP_YUV2RGB_CFG_REG_t *vipp_yuv2rgb_cfg3;
	VIPP_YUV2RGB_CFG_REG_t *vipp_yuv2rgb_cfg4;
	VIPP_YUV2RGB_CFG_REG_t *vipp_yuv2rgb_cfg5;
};
struct vipp_reg vipp_reg_load_addr[VIN_MAX_SCALER];

int vipp_set_base_addr(unsigned int id, unsigned long addr)
{
	if (id > VIN_MAX_SCALER - 1)
		return -1;
	vipp_base[id] = (volatile void __iomem *)addr;

	vipp_base[id] += vipp_virtual_find_ch[id] * addr_base_offset;

	return 0;
}

/*
 * Detail information of top function
 */
void vipp_top_clk_en(unsigned int id, unsigned int en)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_TOP_EN_REG_OFF,
			VIPP_CLK_GATING_EN_MASK, en << VIPP_CLK_GATING_EN);
}

void vipp_cap_enable(unsigned int id)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_TOP_EN_REG_OFF,
			VIPP_CAP_EN_MASK, 1 << VIPP_CAP_EN);
}

void vipp_cap_disable(unsigned int id)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_TOP_EN_REG_OFF,
			VIPP_CAP_EN_MASK, 0 << VIPP_CAP_EN);
}

void vipp_ver_en(unsigned int id, unsigned int en)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_TOP_EN_REG_OFF,
			VIPP_VER_EN_MASK, en << VIPP_VER_EN);
}

void vipp_version_get(unsigned int id, struct vipp_version *v)
{
	unsigned int reg_val = vin_reg_readl(vipp_base[id] + VIPP_VER_REG_OFF);

	v->ver_small = (reg_val & VIPP_SMALL_VER_MASK) >> VIPP_SMALL_VER;
	v->ver_big = (reg_val & VIPP_BIG_VER_MASK) >> VIPP_BIG_VER;
}

void vipp_work_mode(unsigned int id, unsigned int mode)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_TOP_EN_REG_OFF,
			VIPP_WORK_MODE_MASK, mode << VIPP_WORK_MODE);
}

void vipp_feature_list_get(unsigned int id, struct vipp_feature_list *fl)
{
	unsigned int reg_val = vin_reg_readl(vipp_base[id] + VIPP_FEATURE_REG_OFF);

	fl->orl_exit = (reg_val & VIPP_ORL_EXIST_MASK) >> VIPP_ORL_EXIST;
	fl->yuv422to420 = (reg_val & VIPP_YUV422TO420_MASK) >> VIPP_YUV422TO420;
}

void vipp_irq_enable(unsigned int id, unsigned int irq_flag)
{
	vin_reg_set(vipp_base[id] + VIPP_INT_BYPASS_REG_OFF, irq_flag);
}

void vipp_irq_disable(unsigned int id, unsigned int irq_flag)
{
	vin_reg_clr(vipp_base[id] + VIPP_INT_BYPASS_REG_OFF, irq_flag);
}

unsigned int vipp_get_irq_en(unsigned int id, unsigned int irq_flag)
{
	unsigned int reg_val = vin_reg_readl(vipp_base[id] + VIPP_INT_BYPASS_REG_OFF);
	unsigned int ret = 0;

	if (irq_flag == CHN0_REG_LOAD_EN)
		ret = (reg_val & VIPP_CHN0_REG_LOAD_EN_MASK) >> VIPP_CHN0_REG_LOAD_EN;
	else if (irq_flag == CHN1_REG_LOAD_EN)
		ret = (reg_val & VIPP_CHN1_REG_LOAD_EN_MASK) >> VIPP_CHN1_REG_LOAD_EN;
	else if (irq_flag == CHN2_REG_LOAD_EN)
		ret = (reg_val & VIPP_CHN2_REG_LOAD_EN_MASK) >> VIPP_CHN2_REG_LOAD_EN;
	else if (irq_flag == CHN3_REG_LOAD_EN)
		ret = (reg_val & VIPP_CHN3_REG_LOAD_EN_MASK) >> VIPP_CHN3_REG_LOAD_EN;

	return ret;
}

void vipp_get_status(unsigned int id, struct vipp_status *status)
{
	unsigned int reg_val = vin_reg_readl(vipp_base[id] + VIPP_INT_STATUS_REG_OFF);

	status->id_lost_pd = (reg_val & VIPP_ID_LOST_PD_MASK) >> VIPP_ID_LOST_PD;
	status->ahb_mbus_w_pd = (reg_val & VIPP_AHB_MBUS_W_PD_MASK) >> VIPP_AHB_MBUS_W_PD;

	status->chn0_reg_load_pd = (reg_val & VIPP_CHN0_REG_LOAD_PD_MASK) >> VIPP_CHN0_REG_LOAD_PD;
	status->chn0_frame_lost_pd = (reg_val & VIPP_CHN0_FRM_LOST_PD_MASK) >> VIPP_CHN0_FRM_LOST_PD;
	status->chn0_hblank_short_pd = (reg_val & VIPP_CHN0_HB_SHORT_PD_MASK) >> VIPP_CHN0_HB_SHORT_PD;
	status->chn0_para_not_ready_pd = (reg_val & VIPP_CHN0_PAPA_NOTREADY_PD_MASK) >> VIPP_CHN0_PAPA_NOTREADY_PD;
	status->chn0_hshort_pd = (reg_val & VIPP_CHN0_HSHORT_PD_MASK) >> VIPP_CHN0_HSHORT_PD;
	status->chn0_vshort_pd = (reg_val & VIPP_CHN0_VSHORT_PD_MASK) >> VIPP_CHN0_VSHORT_PD;

	status->chn1_reg_load_pd = (reg_val & VIPP_CHN1_REG_LOAD_PD_MASK) >> VIPP_CHN1_REG_LOAD_PD;
	status->chn1_frame_lost_pd = (reg_val & VIPP_CHN1_FRM_LOST_PD_MASK) >> VIPP_CHN1_FRM_LOST_PD;
	status->chn1_hblank_short_pd = (reg_val & VIPP_CHN1_HB_SHORT_PD_MASK) >> VIPP_CHN1_HB_SHORT_PD;
	status->chn1_para_not_ready_pd = (reg_val & VIPP_CHN1_PAPA_NOTREADY_PD_MASK) >> VIPP_CHN1_PAPA_NOTREADY_PD;
	status->chn1_hshort_pd = (reg_val & VIPP_CHN1_HSHORT_PD_MASK) >> VIPP_CHN1_HSHORT_PD;
	status->chn1_vshort_pd = (reg_val & VIPP_CHN1_VSHORT_PD_MASK) >> VIPP_CHN1_VSHORT_PD;

	status->chn2_reg_load_pd = (reg_val & VIPP_CHN2_REG_LOAD_PD_MASK) >> VIPP_CHN2_REG_LOAD_PD;
	status->chn2_frame_lost_pd = (reg_val & VIPP_CHN2_FRM_LOST_PD_MASK) >> VIPP_CHN2_FRM_LOST_PD;
	status->chn2_hblank_short_pd = (reg_val & VIPP_CHN2_HB_SHORT_PD_MASK) >> VIPP_CHN2_HB_SHORT_PD;
	status->chn2_para_not_ready_pd = (reg_val & VIPP_CHN2_PAPA_NOTREADY_PD_MASK) >> VIPP_CHN2_PAPA_NOTREADY_PD;
	status->chn2_hshort_pd = (reg_val & VIPP_CHN2_HSHORT_PD_MASK) >> VIPP_CHN2_HSHORT_PD;
	status->chn2_vshort_pd = (reg_val & VIPP_CHN2_VSHORT_PD_MASK) >> VIPP_CHN2_VSHORT_PD;

	status->chn3_reg_load_pd = (reg_val & VIPP_CHN3_REG_LOAD_PD_MASK) >> VIPP_CHN3_REG_LOAD_PD;
	status->chn3_frame_lost_pd = (reg_val & VIPP_CHN3_FRM_LOST_PD_MASK) >> VIPP_CHN3_FRM_LOST_PD;
	status->chn3_hblank_short_pd = (reg_val & VIPP_CHN3_HB_SHORT_PD_MASK) >> VIPP_CHN3_HB_SHORT_PD;
	status->chn3_para_not_ready_pd = (reg_val & VIPP_CHN3_PAPA_NOTREADY_PD_MASK) >> VIPP_CHN3_PAPA_NOTREADY_PD;
	status->chn3_hshort_pd = (reg_val & VIPP_CHN3_HSHORT_PD_MASK) >> VIPP_CHN3_HSHORT_PD;
	status->chn3_vshort_pd = (reg_val & VIPP_CHN3_VSHORT_PD_MASK) >> VIPP_CHN3_VSHORT_PD;
}

void vipp_clear_status(unsigned int id, enum vipp_status_sel sel)
{
	vin_reg_writel(vipp_base[id] + VIPP_INT_STATUS_REG_OFF, sel);
}

/*
 * Detail information of chn function
 */
static void __vipp_chn_cap_enable(unsigned int id, unsigned int ch)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_CH_CTRL_REG_OFF + VIPP_CH_OFFSET + ch * VIPP_CH_AMONG_OFFSET,
			VIPP_CHN_CAP_EN_MASK, 1 << VIPP_CHN_CAP_EN);
}

void vipp_chn_cap_enable(unsigned int id)
{
	__vipp_chn_cap_enable(id, vipp_virtual_find_ch[id]);
}

static void __vipp_chn_cap_disable(unsigned int id, unsigned int ch)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_CH_CTRL_REG_OFF + VIPP_CH_OFFSET + ch * VIPP_CH_AMONG_OFFSET,
			VIPP_CHN_CAP_EN_MASK, 0 << VIPP_CHN_CAP_EN);
}

void vipp_chn_cap_disable(unsigned int id)
{
	__vipp_chn_cap_disable(id, vipp_virtual_find_ch[id]);
}

static void __vipp_set_para_ready(unsigned int id, enum vipp_ready_flag flag, unsigned int ch)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_CH_CTRL_REG_OFF + VIPP_CH_OFFSET + ch * VIPP_CH_AMONG_OFFSET,
			VIPP_PARA_READY_MASK, flag << VIPP_PARA_READY);
}

void vipp_set_para_ready(unsigned int id, enum vipp_ready_flag flag)
{
#ifndef	VIPP_SCALER_DIRECTLY_WRITE_REG
	__vipp_set_para_ready(id, flag, vipp_virtual_find_ch[id]);
#endif
}

static void __vipp_chn_cap_disable_para_notready(unsigned int id, unsigned int ch)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_CH_CTRL_REG_OFF + VIPP_CH_OFFSET + ch * VIPP_CH_AMONG_OFFSET,
			VIPP_CHN_CAP_EN_MASK | VIPP_PARA_READY_MASK, (0 << VIPP_CHN_CAP_EN) | (0 << VIPP_PARA_READY));
}

void vipp_chn_cap_disable_para_notready(unsigned int id)
{
	__vipp_chn_cap_disable_para_notready(id, vipp_virtual_find_ch[id]);
}

static void __vipp_chn_bypass_mode(unsigned int id, unsigned int mode, unsigned int ch)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_CH_CTRL_REG_OFF + VIPP_CH_OFFSET + ch * VIPP_CH_AMONG_OFFSET,
			VIPP_BYPASS_MODE_MASK, mode << VIPP_BYPASS_MODE);
}

void vipp_chn_bypass_mode(unsigned int id, unsigned int mode)
{
	__vipp_chn_bypass_mode(id, mode, vipp_virtual_find_ch[id]);
}

static void __vipp_set_reg_load_addr(unsigned int id, unsigned long dma_addr, unsigned int ch)
{
	vin_reg_writel(vipp_base[id] + VIPP_REG_LOAD_ADDR_REG_OFF + VIPP_CH_OFFSET + ch * VIPP_CH_AMONG_OFFSET,
			dma_addr >> VIPP_ADDR_BIT_R_SHIFT);
}

void vipp_set_reg_load_addr(unsigned int id, unsigned long dma_addr)
{
	__vipp_set_reg_load_addr(id, dma_addr, vipp_virtual_find_ch[id]);
}

/*
 * Detail information of load function
 */
int vipp_map_reg_load_addr(unsigned int id, unsigned long vaddr)
{
	if (id > VIN_MAX_SCALER - 1)
		return -1;

	vipp_reg_load_addr[id].vipp_module_en = (VIPP_MODULE_EN_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_MODULE_EN_REG_OFF);
	vipp_reg_load_addr[id].vipp_scaler_cfg = (VIPP_SCALER_CFG_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_SC_CFG_REG_OFF);
	vipp_reg_load_addr[id].vipp_scaler_output_size = (VIPP_SCALER_OUTPUT_SIZE_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_SC_SIZE_REG_OFF);
	vipp_reg_load_addr[id].vipp_output_fmt = (VIPP_OUTPUT_FMT_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_MODE_REG_OFF);
	vipp_reg_load_addr[id].vipp_crop_start = (VIPP_CROP_START_POSITION_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_CROP_START_REG_OFF);
	vipp_reg_load_addr[id].vipp_cgc_gain_ctrl = (VIPP_CGC_GAIN_CTRL_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_CGC_GAIN_CTRL_REG_OFF);
	vipp_reg_load_addr[id].vipp_cgc_clip_ctrl = (VIPP_CGC_CLIP_CTRL_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_CGC_CLIP_CTRL_REG);
	vipp_reg_load_addr[id].vipp_crop_size = (VIPP_CROP_SIZE_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_CROP_SIZE_REG_OFF);
	vipp_reg_load_addr[id].vipp_orl_control = (VIPP_ORL_CONTROL_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_ORL_CONTROL_REG_OFF);
	vipp_reg_load_addr[id].vipp_orl_start = (VIPP_ORL_START_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_ORL_START0_REG_OFF);
	vipp_reg_load_addr[id].vipp_orl_end = (VIPP_ORL_END_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_ORL_END0_REG_OFF);
	vipp_reg_load_addr[id].vipp_orl_yuv = (VIPP_ORL_YUV_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_ORL_YUV0_REG_OFF);

	vipp_reg_load_addr[id].vipp_yuv2rgb_cfg0 = (VIPP_YUV2RGB_CFG_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_YUV2RGB_CFG0_REG_OFF);
	vipp_reg_load_addr[id].vipp_yuv2rgb_cfg1 = (VIPP_YUV2RGB_CFG_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_YUV2RGB_CFG1_REG_OFF);
	vipp_reg_load_addr[id].vipp_yuv2rgb_cfg2 = (VIPP_YUV2RGB_CFG_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_YUV2RGB_CFG2_REG_OFF);
	vipp_reg_load_addr[id].vipp_yuv2rgb_cfg3 = (VIPP_YUV2RGB_CFG_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_YUV2RGB_CFG3_REG_OFF);
	vipp_reg_load_addr[id].vipp_yuv2rgb_cfg4 = (VIPP_YUV2RGB_CFG_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_YUV2RGB_CFG4_REG_OFF);
	vipp_reg_load_addr[id].vipp_yuv2rgb_cfg5 = (VIPP_YUV2RGB_CFG_REG_t *)(vaddr + VIPP_LOAD_OFFSET + VIPP_YUV2RGB_CFG5_REG_OFF);
	return 0;
}

void vipp_cgc_gain_cfg(unsigned int id, int val[][4], int rows)
{
	/* default value */
	vipp_reg_load_addr[id].vipp_cgc_gain_ctrl->bits.cgc_f2l_gain_y = val[rows][0];
	vipp_reg_load_addr[id].vipp_cgc_gain_ctrl->bits.cgc_f2l_gain_uv = val[rows][1];
	vipp_reg_load_addr[id].vipp_cgc_gain_ctrl->bits.cgc_f2l_offset_y = val[rows][2];
	vipp_reg_load_addr[id].vipp_cgc_gain_ctrl->bits.cgc_f2l_offset_uv = val[rows][3];

}

void vipp_cgc_clip_cfg(unsigned int id, int val[][4], int rows)
{
	/* default value */
	vipp_reg_load_addr[id].vipp_cgc_clip_ctrl->bits.cgc_f2l_y_min = val[rows][0];
	vipp_reg_load_addr[id].vipp_cgc_clip_ctrl->bits.cgc_f2l_y_max = val[rows][1];
	vipp_reg_load_addr[id].vipp_cgc_clip_ctrl->bits.cgc_f2l_uv_min = val[rows][2];
	vipp_reg_load_addr[id].vipp_cgc_clip_ctrl->bits.cgc_f2l_uv_max = val[rows][3];
}

void vipp_cgc_f2l_en(unsigned int id, unsigned int en)
{
	vipp_reg_load_addr[id].vipp_module_en->bits.cgc_f2l_en = en;
}

void vipp_yuv2rgb_coef_cfg(unsigned int id)
{
	/* default value */
	vipp_reg_load_addr[id].vipp_yuv2rgb_cfg0->dwval = 0x0000012a;
	vipp_reg_load_addr[id].vipp_yuv2rgb_cfg1->dwval = 0x012a0199;
	vipp_reg_load_addr[id].vipp_yuv2rgb_cfg2->dwval = 0x0730079c;
	vipp_reg_load_addr[id].vipp_yuv2rgb_cfg3->dwval = 0x0204012a;
	vipp_reg_load_addr[id].vipp_yuv2rgb_cfg4->dwval = 0x07210000;
	vipp_reg_load_addr[id].vipp_yuv2rgb_cfg5->dwval = 0x06ec0087;
}

void vipp_yuv2rgb_en(unsigned int id, unsigned int en)
{
	vipp_reg_load_addr[id].vipp_module_en->bits.yuv2rgb_en = en;
}

void vipp_scaler_en(unsigned int id, unsigned int en)
{
#ifndef	VIPP_SCALER_DIRECTLY_WRITE_REG
	vipp_reg_load_addr[id].vipp_module_en->bits.sc_en = en;
#else
	VIPP_MODULE_EN_REG_t vipp_module_en;

	vipp_module_en.dwval = 0;
	vipp_module_en.bits.sc_en = en;
	vin_reg_writel(vipp_base[id] + VIPP_LOAD_OFFSET + VIPP_MODULE_EN_REG_OFF, vipp_module_en.dwval);
#endif
}
void vipp_chroma_ds_en(unsigned int id, unsigned int en)
{
	if (id > MAX_OSD_NUM - 1)
		return;

	vipp_reg_load_addr[id].vipp_module_en->bits.chroma_ds_en = en;
}

void vipp_scaler_cfg(unsigned int id, struct vipp_scaler_config *cfg)
{
#ifndef	VIPP_SCALER_DIRECTLY_WRITE_REG
	vipp_reg_load_addr[id].vipp_output_fmt->bits.sc_out_fmt = cfg->sc_out_fmt;
	vipp_reg_load_addr[id].vipp_scaler_cfg->bits.sc_xratio = cfg->sc_x_ratio;
	vipp_reg_load_addr[id].vipp_scaler_cfg->bits.sc_yratio = cfg->sc_y_ratio;
	vipp_reg_load_addr[id].vipp_scaler_cfg->bits.sc_weight_shift = cfg->sc_w_shift;
#else
	VIPP_SCALER_CFG_REG_t vipp_scaler_cfg;

	vipp_scaler_cfg.dwval = 0;
	vipp_scaler_cfg.bits.sc_xratio = cfg->sc_x_ratio;
	vipp_scaler_cfg.bits.sc_yratio = cfg->sc_y_ratio;
	vipp_scaler_cfg.bits.sc_weight_shift = cfg->sc_w_shift;
	vin_reg_writel(vipp_base[id] + VIPP_LOAD_OFFSET + VIPP_SC_CFG_REG_OFF, vipp_scaler_cfg.dwval);
	vin_reg_clr_set(vipp_base[id] + VIPP_LOAD_OFFSET + VIPP_MODE_REG_OFF, 0x1 << 2, cfg->sc_out_fmt << 2);
#endif
}

void vipp_scaler_output_fmt(unsigned int id, enum vipp_format fmt)
{
#ifndef	VIPP_SCALER_DIRECTLY_WRITE_REG
	vipp_reg_load_addr[id].vipp_output_fmt->bits.sc_out_fmt = fmt;
#else
	vin_reg_clr_set(vipp_base[id] + VIPP_LOAD_OFFSET + VIPP_MODE_REG_OFF,  0x1 << 2, fmt << 2);
#endif
}

void vipp_scaler_output_size(unsigned int id, struct vipp_scaler_size *size)
{
#ifndef	VIPP_SCALER_DIRECTLY_WRITE_REG
	vipp_reg_load_addr[id].vipp_scaler_output_size->bits.sc_width = size->sc_width;
	vipp_reg_load_addr[id].vipp_scaler_output_size->bits.sc_height = size->sc_height;
#else
	VIPP_SCALER_OUTPUT_SIZE_REG_t vipp_scaler_output_size;

	vipp_scaler_output_size.dwval = 0;
	vipp_scaler_output_size.bits.sc_width = size->sc_width;
	vipp_scaler_output_size.bits.sc_height = size->sc_height;
	vin_reg_writel(vipp_base[id] + VIPP_LOAD_OFFSET + VIPP_SC_SIZE_REG_OFF, vipp_scaler_output_size.dwval);
#endif
}

void vipp_output_fmt_cfg(unsigned int id, enum vipp_format fmt)
{
#ifndef	VIPP_SCALER_DIRECTLY_WRITE_REG
	vipp_reg_load_addr[id].vipp_output_fmt->bits.vipp_out_fmt = fmt;
	vipp_reg_load_addr[id].vipp_output_fmt->bits.vipp_in_fmt = 1;

#else
	vin_reg_clr_set(vipp_base[id] + VIPP_LOAD_OFFSET + VIPP_MODE_REG_OFF, 0x1, fmt);
	vin_reg_clr_set(vipp_base[id] + VIPP_LOAD_OFFSET + VIPP_MODE_REG_OFF, 0x1 << 1, 1 << 1);
#endif
}

void vipp_osd_cfg(unsigned int id, struct vipp_osd_config *cfg)
{
	vipp_reg_load_addr[id].vipp_orl_control->bits.orl_num = cfg->osd_orl_num + 1;
	vipp_reg_load_addr[id].vipp_orl_control->bits.orl_width = cfg->osd_orl_width;
}

void vipp_set_crop(unsigned int id, struct vipp_crop *crop)
{
	vipp_reg_load_addr[id].vipp_crop_start->bits.crop_hor_st = crop->hor;
	vipp_reg_load_addr[id].vipp_crop_start->bits.crop_ver_st = crop->ver;
	vipp_reg_load_addr[id].vipp_crop_size->bits.crop_width = crop->width;
	vipp_reg_load_addr[id].vipp_crop_size->bits.crop_height = crop->height;
}

void vipp_osd_para_cfg(unsigned int id, struct vipp_osd_para_config *para,
				struct vipp_osd_config *cfg)
{
	int i;

	for (i = 0; i < cfg->osd_orl_num + 1; i++) {
		vipp_reg_load_addr[id].vipp_orl_start[i].bits.orl_ys = para->orl_cfg[i].v_start;
		vipp_reg_load_addr[id].vipp_orl_start[i].bits.orl_xs = para->orl_cfg[i].h_start;
		vipp_reg_load_addr[id].vipp_orl_end[i].bits.orl_ye = para->orl_cfg[i].v_end;
		vipp_reg_load_addr[id].vipp_orl_end[i].bits.orl_xe = para->orl_cfg[i].h_end;
		vipp_reg_load_addr[id].vipp_orl_yuv[i].bits.orl_y = para->orl_data[i].y;
		vipp_reg_load_addr[id].vipp_orl_yuv[i].bits.orl_u = para->orl_data[i].u;
		vipp_reg_load_addr[id].vipp_orl_yuv[i].bits.orl_v = para->orl_data[i].v;
	}
}

/*
 * Detail information of vipp100 function
 */
void vipp_set_osd_ov_update(unsigned int id, enum vipp_update_flag flag)
{
}
void vipp_set_osd_cv_update(unsigned int id, enum vipp_update_flag flag)
{
}
void vipp_set_osd_para_load_addr(unsigned int id, unsigned long dma_addr)
{
}
int vipp_map_osd_para_load_addr(unsigned int id, unsigned long vaddr)
{
	return 0;
}
void vipp_set_osd_stat_load_addr(unsigned int id, unsigned long dma_addr)
{
}
void vipp_set_osd_bm_load_addr(unsigned int id, unsigned long dma_addr)
{
}
void vipp_osd_en(unsigned int id, unsigned int en)
{
}
void vipp_osd_rgb2yuv(unsigned int id, struct vipp_rgb2yuv_factor *factor)
{
}
void vipp_osd_hvflip(unsigned int id, int hflip, int vflip)
{
}

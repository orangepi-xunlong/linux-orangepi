/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-vipp/vipp_reg.c
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
 * vipp_reg.c
 *
 * VIPP - vipp_reg.c module
 *
 * Copyright (c) 2016 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version          Author         Date        Description
 *
 *   1.0          Zhao Wei     2016/07/19     First Version
 *
 ******************************************************************************
 */

#include <linux/kernel.h>
#include "vipp_reg_i.h"
#include "vipp_reg.h"

#include "../utility/vin_io.h"

/*#define VIPP_SCALER_DIRECTLY_WRITE_REG*/
#define VIPP_ADDR_BIT_R_SHIFT 2

volatile void __iomem *vipp_base[MAX_VIPP_NUM];

struct vipp_reg {
	VIPP_MODULE_EN_REG_t *vipp_module_en;
	VIPP_SCALER_CFG_REG_t *vipp_scaler_cfg;
	VIPP_SCALER_OUTPUT_SIZE_REG_t *vipp_scaler_output_size;
	VIPP_OUTPUT_FMT_REG_t *vipp_output_fmt;
	VIPP_OSD_CFG_REG_t *vipp_osd_cfg;
	VIPP_OSD_RGB2YUV_GAIN0_REG_t *vipp_osd_rgb2yuv_gain0;
	VIPP_OSD_RGB2YUV_GAIN1_REG_t *vipp_osd_rgb2yuv_gain1;
	VIPP_OSD_RGB2YUV_GAIN2_REG_t *vipp_osd_rgb2yuv_gain2;
	VIPP_OSD_RGB2YUV_GAIN3_REG_t *vipp_osd_rgb2yuv_gain3;
	VIPP_OSD_RGB2YUV_GAIN4_REG_t *vipp_osd_rgb2yuv_gain4;
	VIPP_OSD_RGB2YUV_OFFSET_REG_t *vipp_osd_rgb2yuv_offset;
	VIPP_CROP_START_POSITION_REG_t *vipp_crop_start;
	VIPP_CROP_SIZE_REG_t *vipp_crop_size;
};
struct vipp_reg vipp_reg_load_addr[MAX_VIPP_NUM];

struct vipp_osd_para {
	VIPP_OSD_OVERLAY_CFG_REG_t *vipp_osd_overlay_cfg;
	VIPP_OSD_COVER_CFG_REG_t *vipp_osd_cover_cfg;
	VIPP_OSD_COVER_DATA_REG_t *vipp_osd_cover_data;
};
struct vipp_osd_para vipp_osd_para_load_addr[MAX_VIPP_NUM];


int vipp_set_base_addr(unsigned int id, unsigned long addr)
{
	if (id > MAX_VIPP_NUM - 1)
		return -1;
	vipp_base[id] = (volatile void __iomem *)addr;

	return 0;
}

/* open module */

void vipp_top_clk_en(unsigned int id, unsigned int en)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_TOP_EN_REG_OFF,
			VIPP_CLK_GATING_EN_MASK, en << VIPP_CLK_GATING_EN);
}

void vipp_enable(unsigned int id)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_EN_REG_OFF,
			VIPP_EN_MASK, 1 << VIPP_EN);
}

void vipp_disable(unsigned int id)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_EN_REG_OFF,
			VIPP_EN_MASK, 0 << VIPP_EN);
}

void vipp_ver_en(unsigned int id, unsigned int en)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_EN_REG_OFF,
			VIPP_VER_EN_MASK, en << VIPP_VER_EN);
}

void vipp_version_get(unsigned int id, struct vipp_version *v)
{
	unsigned int reg_val = vin_reg_readl(vipp_base[id] + VIPP_VER_REG_OFF);

	v->ver_small = (reg_val & VIPP_SMALL_VER_MASK) >> VIPP_SMALL_VER;
	v->ver_big = (reg_val & VIPP_BIG_VER_MASK) >> VIPP_BIG_VER;
}

void vipp_feature_list_get(unsigned int id, struct vipp_feature_list *fl)
{
	unsigned int reg_val = vin_reg_readl(vipp_base[id] + VIPP_FEATURE_REG_OFF);

	fl->osd_exit = (reg_val & VIPP_OSD_EXIST_MASK) >> VIPP_OSD_EXIST;
	fl->fbc_exit = (reg_val & VIPP_FBC_EXIST_MASK) >> VIPP_FBC_EXIST;
}

int vipp_get_para_ready(unsigned int id)
{
	unsigned int reg_val = vin_reg_readl(vipp_base[id] + VIPP_CTRL_REG_OFF);

	return (reg_val & VIPP_PARA_READY_MASK) >> VIPP_PARA_READY;
}

void vipp_set_para_ready(unsigned int id, enum vipp_ready_flag flag)
{
#ifndef VIPP_SCALER_DIRECTLY_WRITE_REG
	vin_reg_clr_set(vipp_base[id] + VIPP_CTRL_REG_OFF,
			VIPP_PARA_READY_MASK, flag << VIPP_PARA_READY);
#endif
}

int vipp_get_osd_ov_update(unsigned int id)
{
	unsigned int reg_val = vin_reg_readl(vipp_base[id] + VIPP_CTRL_REG_OFF);

	return (reg_val & VIPP_OSD_OV_UPDATE_MASK) >> VIPP_OSD_OV_UPDATE;
}

void vipp_set_osd_ov_update(unsigned int id, enum vipp_update_flag flag)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_CTRL_REG_OFF,
			VIPP_OSD_OV_UPDATE_MASK, flag << VIPP_OSD_OV_UPDATE);
}

int vipp_get_osd_cv_update(unsigned int id)
{
	unsigned int reg_val = vin_reg_readl(vipp_base[id] + VIPP_CTRL_REG_OFF);

	return (reg_val & VIPP_OSD_CV_UPDATE_MASK) >> VIPP_OSD_CV_UPDATE;
}

void vipp_set_osd_cv_update(unsigned int id, enum vipp_update_flag flag)
{
	vin_reg_clr_set(vipp_base[id] + VIPP_CTRL_REG_OFF,
			VIPP_OSD_CV_UPDATE_MASK, flag << VIPP_OSD_CV_UPDATE);
}

void vipp_set_osd_para_load_addr(unsigned int id, unsigned long dma_addr)
{
	vin_reg_writel(vipp_base[id] + VIPP_OSD_LOAD_ADDR_REG_OFF, dma_addr >> VIPP_ADDR_BIT_R_SHIFT);
}

int vipp_map_osd_para_load_addr(unsigned int id, unsigned long vaddr)
{
	if (id > MAX_VIPP_NUM - 1)
		return -1;

	vipp_osd_para_load_addr[id].vipp_osd_overlay_cfg = (VIPP_OSD_OVERLAY_CFG_REG_t *)vaddr;
	vipp_osd_para_load_addr[id].vipp_osd_cover_cfg = (VIPP_OSD_COVER_CFG_REG_t *)(vaddr + MAX_OVERLAY_NUM * 8);
	vipp_osd_para_load_addr[id].vipp_osd_cover_data = (VIPP_OSD_COVER_DATA_REG_t *)(vaddr + MAX_OVERLAY_NUM * 8 + MAX_COVER_NUM * 8);

	return 0;
}

void vipp_set_osd_stat_load_addr(unsigned int id, unsigned long dma_addr)
{
	vin_reg_writel(vipp_base[id] + VIPP_OSD_STAT_ADDR_REG_OFF, dma_addr >> VIPP_ADDR_BIT_R_SHIFT);
}

void vipp_set_osd_bm_load_addr(unsigned int id, unsigned long dma_addr)
{
	vin_reg_writel(vipp_base[id] + VIPP_OSD_BM_ADDR_REG_OFF, dma_addr >> VIPP_ADDR_BIT_R_SHIFT);
}

void vipp_set_reg_load_addr(unsigned int id, unsigned long dma_addr)
{
	vin_reg_writel(vipp_base[id] + VIPP_REG_LOAD_ADDR_REG_OFF, dma_addr >> VIPP_ADDR_BIT_R_SHIFT);
}

int vipp_map_reg_load_addr(unsigned int id, unsigned long vaddr)
{
	if (id > MAX_VIPP_NUM - 1)
		return -1;

	vipp_reg_load_addr[id].vipp_module_en = (VIPP_MODULE_EN_REG_t *)(vaddr + VIPP_MODULE_EN_REG_OFF);
	vipp_reg_load_addr[id].vipp_scaler_cfg = (VIPP_SCALER_CFG_REG_t *)(vaddr + VIPP_SC_CFG_REG_OFF);
	vipp_reg_load_addr[id].vipp_scaler_output_size = (VIPP_SCALER_OUTPUT_SIZE_REG_t *)(vaddr + VIPP_SC_SIZE_REG_OFF);
	vipp_reg_load_addr[id].vipp_output_fmt = (VIPP_OUTPUT_FMT_REG_t *)(vaddr + VIPP_MODE_REG_OFF);
	vipp_reg_load_addr[id].vipp_osd_cfg = (VIPP_OSD_CFG_REG_t *)(vaddr + VIPP_OSD_CFG_REG_OFF);
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain0 = (VIPP_OSD_RGB2YUV_GAIN0_REG_t *)(vaddr + VIPP_OSD_GAIN0_REG_OFF);
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain1 = (VIPP_OSD_RGB2YUV_GAIN1_REG_t *)(vaddr + VIPP_OSD_GAIN1_REG_OFF);
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain2 = (VIPP_OSD_RGB2YUV_GAIN2_REG_t *)(vaddr + VIPP_OSD_GAIN2_REG_OFF);
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain3 = (VIPP_OSD_RGB2YUV_GAIN3_REG_t *)(vaddr + VIPP_OSD_GAIN3_REG_OFF);
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain4 = (VIPP_OSD_RGB2YUV_GAIN4_REG_t *)(vaddr + VIPP_OSD_GAIN4_REG_OFF);
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_offset = (VIPP_OSD_RGB2YUV_OFFSET_REG_t *)(vaddr + VIPP_OSD_OFFSET_REG_OFF);
	vipp_reg_load_addr[id].vipp_crop_start = (VIPP_CROP_START_POSITION_REG_t *)(vaddr + VIPP_CROP_START_REG_OFF);
	vipp_reg_load_addr[id].vipp_crop_size = (VIPP_CROP_SIZE_REG_t *)(vaddr + VIPP_CROP_SIZE_REG_OFF);

	return 0;
}

void vipp_get_status(unsigned int id, struct vipp_status *status)
{
	unsigned int reg_val = vin_reg_readl(vipp_base[id] + VIPP_STA_REG_OFF);

	status->reg_load_pd = (reg_val & VIPP_REG_LOAD_PD_MASK) >> VIPP_REG_LOAD_PD;
	status->bm_error_pd = (reg_val & VIPP_BM_ERROR_PD_MASK) >> VIPP_BM_ERROR_PD;
}

void vipp_clr_status(unsigned int id, enum vipp_status_sel sel)
{
	vin_reg_writel(vipp_base[id] + VIPP_STA_REG_OFF, sel);
}

void vipp_scaler_en(unsigned int id, unsigned int en)
{
#ifndef	VIPP_SCALER_DIRECTLY_WRITE_REG
	vipp_reg_load_addr[id].vipp_module_en->bits.sc_en = en;
#else
	VIPP_MODULE_EN_REG_t vipp_module_en;

	vipp_module_en.dwval = 0;
	vipp_module_en.bits.sc_en = en;
	vin_reg_writel(vipp_base[id] + VIPP_MODULE_EN_REG_OFF, vipp_module_en.dwval);
#endif
}

void vipp_osd_en(unsigned int id, unsigned int en)
{
	vipp_reg_load_addr[id].vipp_module_en->bits.osd_en = en;
}

void vipp_chroma_ds_en(unsigned int id, unsigned int en)
{
	vipp_reg_load_addr[id].vipp_module_en->bits.chroma_ds_en = en;
}

void vipp_scaler_cfg(unsigned int id, struct vipp_scaler_config *cfg)
{
#ifndef	VIPP_SCALER_DIRECTLY_WRITE_REG
	vipp_reg_load_addr[id].vipp_scaler_cfg->bits.sc_out_fmt = cfg->sc_out_fmt;
	vipp_reg_load_addr[id].vipp_scaler_cfg->bits.sc_xratio = cfg->sc_x_ratio;
	vipp_reg_load_addr[id].vipp_scaler_cfg->bits.sc_yratio = cfg->sc_y_ratio;
	vipp_reg_load_addr[id].vipp_scaler_cfg->bits.sc_weight_shift = cfg->sc_w_shift;
#else
	VIPP_SCALER_CFG_REG_t vipp_scaler_cfg;

	vipp_scaler_cfg.dwval = 0;
	vipp_scaler_cfg.bits.sc_out_fmt = cfg->sc_out_fmt;
	vipp_scaler_cfg.bits.sc_xratio = cfg->sc_x_ratio;
	vipp_scaler_cfg.bits.sc_yratio = cfg->sc_y_ratio;
	vipp_scaler_cfg.bits.sc_weight_shift = cfg->sc_w_shift;
	vin_reg_writel(vipp_base[id] + VIPP_SC_CFG_REG_OFF, vipp_scaler_cfg.dwval);

#endif
}

void vipp_scaler_output_fmt(unsigned int id, enum vipp_format fmt)
{
#ifndef	VIPP_SCALER_DIRECTLY_WRITE_REG
	vipp_reg_load_addr[id].vipp_scaler_cfg->bits.sc_out_fmt = fmt;
#else
	vin_reg_clr_set(vipp_base[id] + VIPP_SC_CFG_REG_OFF, 0x1, fmt);
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
	vin_reg_writel(vipp_base[id] + VIPP_SC_SIZE_REG_OFF, vipp_scaler_output_size.dwval);
#endif
}

void vipp_output_fmt_cfg(unsigned int id, enum vipp_format fmt)
{
#ifndef	VIPP_SCALER_DIRECTLY_WRITE_REG
	vipp_reg_load_addr[id].vipp_output_fmt->bits.vipp_out_fmt = fmt;
	vipp_reg_load_addr[id].vipp_output_fmt->bits.vipp_in_fmt = 1;

#else
	VIPP_OUTPUT_FMT_REG_t vipp_output_fmt;

	vipp_output_fmt.dwval = 0;
	vipp_output_fmt.bits.vipp_out_fmt = fmt;
	vipp_output_fmt.bits.vipp_in_fmt = 1;
	vin_reg_writel(vipp_base[id] + VIPP_MODE_REG_OFF, vipp_output_fmt.dwval);
#endif
}

void vipp_osd_cfg(unsigned int id, struct vipp_osd_config *cfg)
{
	vipp_reg_load_addr[id].vipp_osd_cfg->bits.osd_ov_en = cfg->osd_ov_en;
	vipp_reg_load_addr[id].vipp_osd_cfg->bits.osd_cv_en = cfg->osd_cv_en;
	vipp_reg_load_addr[id].vipp_osd_cfg->bits.osd_argb_mode = cfg->osd_argb_mode;
	vipp_reg_load_addr[id].vipp_osd_cfg->bits.osd_stat_en = cfg->osd_stat_en;
	vipp_reg_load_addr[id].vipp_osd_cfg->bits.osd_ov_num = cfg->osd_ov_num;
	vipp_reg_load_addr[id].vipp_osd_cfg->bits.osd_cv_num = cfg->osd_cv_num;
}

void vipp_osd_rgb2yuv(unsigned int id, struct vipp_rgb2yuv_factor *factor)
{
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain0->bits.jc0 = factor->jc0;
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain0->bits.jc1 = factor->jc1;
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain1->bits.jc2 = factor->jc2;
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain1->bits.jc3 = factor->jc3;
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain2->bits.jc4 = factor->jc4;
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain2->bits.jc5 = factor->jc5;
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain3->bits.jc6 = factor->jc6;
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain3->bits.jc7 = factor->jc7;
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_gain4->bits.jc8 = factor->jc8;
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_offset->bits.jc9 = factor->jc9;
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_offset->bits.jc10 = factor->jc10;
	vipp_reg_load_addr[id].vipp_osd_rgb2yuv_offset->bits.jc11 = factor->jc11;
}

void vipp_set_crop(unsigned int id, struct vipp_crop *crop)
{
	vipp_reg_load_addr[id].vipp_crop_start->bits.crop_hor_st = crop->hor;
	vipp_reg_load_addr[id].vipp_crop_start->bits.crop_ver_st = crop->ver;
	vipp_reg_load_addr[id].vipp_crop_size->bits.crop_width = crop->width;
	vipp_reg_load_addr[id].vipp_crop_size->bits.crop_height = crop->height;
}

void vipp_osd_inverse(unsigned int id, int *inverse, int cnt)
{
	int i;

	for (i = 0; i < cnt; i++)
		vipp_osd_para_load_addr[id].vipp_osd_overlay_cfg[i].bits.inverse_en = inverse[i];
}

void vipp_osd_para_cfg(unsigned int id, struct vipp_osd_para_config *para,
				struct vipp_osd_config *cfg)
{
	int i;

	for (i = 0; i < cfg->osd_ov_num + 1; i++) {
		vipp_osd_para_load_addr[id].vipp_osd_overlay_cfg[i].bits.h_start = para->overlay_cfg[i].h_start;
		vipp_osd_para_load_addr[id].vipp_osd_overlay_cfg[i].bits.h_end = para->overlay_cfg[i].h_end;
		vipp_osd_para_load_addr[id].vipp_osd_overlay_cfg[i].bits.v_start = para->overlay_cfg[i].v_start;
		vipp_osd_para_load_addr[id].vipp_osd_overlay_cfg[i].bits.v_end = para->overlay_cfg[i].v_end;
		vipp_osd_para_load_addr[id].vipp_osd_overlay_cfg[i].bits.alpha = para->overlay_cfg[i].alpha;
	}

	for (i = 0; i < cfg->osd_cv_num + 1; i++) {
		vipp_osd_para_load_addr[id].vipp_osd_cover_cfg[i].bits.h_start = para->cover_cfg[i].h_start;
		vipp_osd_para_load_addr[id].vipp_osd_cover_cfg[i].bits.h_end = para->cover_cfg[i].h_end;
		vipp_osd_para_load_addr[id].vipp_osd_cover_cfg[i].bits.v_start = para->cover_cfg[i].v_start;
		vipp_osd_para_load_addr[id].vipp_osd_cover_cfg[i].bits.v_end = para->cover_cfg[i].v_end;
		vipp_osd_para_load_addr[id].vipp_osd_cover_data[i].bits.y = para->cover_data[i].y;
		vipp_osd_para_load_addr[id].vipp_osd_cover_data[i].bits.u = para->cover_data[i].u;
		vipp_osd_para_load_addr[id].vipp_osd_cover_data[i].bits.v = para->cover_data[i].v;
	}
}

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
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
#include "dma140_reg_i.h"
#include "dma140_reg.h"

#include "../../utility/vin_io.h"
#include "../../platform/platform_cfg.h"

#define ADDR_BIT_R_SHIFT 2

volatile void __iomem *csic_dma_base[VIN_MAX_SCALER];

#define addr_base_offset 0x4

#if IS_ENABLED(CONFIG_ARCH_SUN60IW1)
int dma_virtual_find_ch[24] = {
	0, 1, 2, 3, 0, 1, 2, 3, 0, 0, 0, 0, 0, 1, 2, 3, 0, 1, 2, 3, 0, 0, 0, 0,
};
int dma_virtual_find_logic[24] = {
	0, 0, 0, 0, 4, 4, 4, 4, 8, 9, 10, 11, 12, 12, 12, 12, 16, 16, 16, 16, 20, 21, 22, 23,
};
#else
int dma_virtual_find_ch[18] = {
	0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 0,
};
int dma_virtual_find_logic[18] = {
	0, 0, 0, 0, 4, 4, 4, 4, 8, 8, 8, 8, 12, 12, 12, 12, 16, 17,
};
#endif

int csic_dma_set_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel > VIN_MAX_SCALER - 1)
		return -1;
	csic_dma_base[sel] = (volatile void __iomem *)addr;
	csic_dma_base[sel] += dma_virtual_find_ch[sel] * addr_base_offset;
	return 0;
}

/* open module */

int csic_dma_get_frame_cnt(unsigned int sel)
{
	return 0;
}

/* BK TOP EN */
void csic_dma_top_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_DMA_TOP_EN_MASK, 1 << CSIC_DMA_TOP_EN);
}

void csic_dma_top_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_DMA_TOP_EN_MASK, 0 << CSIC_DMA_TOP_EN);
}

void csic_dma_clk_cnt_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_CLK_CNT_EN_MASK, 1 << CSIC_CLK_CNT_EN);
}
void csic_dma_clk_cnt_sample(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_CLK_CNT_SPL_MASK, 1 << CSIC_CLK_CNT_SPL);
}

void csic_frame_cnt_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_FRAME_CNT_EN_MASK, 1 << CSIC_FRAME_CNT_EN);
}

void csic_frame_cnt_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_FRAME_CNT_EN_MASK, 0 << CSIC_FRAME_CNT_EN);
}

void csic_vi_to_cnt_enable(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_VI_TO_CNT_EN_MASK, 1 << CSIC_VI_TO_CNT_EN);
}

void csic_dma_sdr_wr_size(unsigned int sel, unsigned int block)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_MIN_SDR_WR_SIZE_MASK, block << CSIC_MIN_SDR_WR_SIZE);
}

void csic_ve_online_hs_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_VE_ONLINE_HS_EN_MASK, 1 << CSIC_VE_ONLINE_HS_EN);

}

void csic_ve_online_hs_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_VE_ONLINE_HS_EN_MASK, 0 << CSIC_VE_ONLINE_HS_EN);

}

void csic_ve_online_ch_sel(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_VE_ONLINE_CH_SEL_MASK, ch << CSIC_VE_ONLINE_CH_SEL);

}

void csic_dma_buf_length_software_enable(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
		CSIC_BUF_LENGTH_CFG_MODE_MASK, en << CSIC_BUF_LENGTH_CFG_MODE);
}

void csi_dam_flip_software_enable(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_FLIP_SIZE_CFG_MODE_MASK, en << CSIC_FLIP_SIZE_CFG_MODE);
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
			CSIC_VFLIP_BUF_ADDR_CFG_MODE_MASK, en << CSIC_VFLIP_BUF_ADDR_CFG_MODE);
}

void csic_dma_version_read_enable(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_TOP_EN_REG_OFF,
		CSIC_VER_EN_MASK, en << CSIC_VER_EN);
}


/** top offset 0x4 **/
void csic_dma_mul_ch_enable(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_MUL_CH_CFG_REG_OFF,
		MUL_CH_EN_MASK, en << MUL_CH_EN);
}

unsigned int csic_get_input_ch_id(unsigned int sel)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_MUL_CH_CFG_REG_OFF);

	return ((reg_val & CUR_IN_CH_MASK) >> CUR_IN_CH);
}

unsigned int csic_get_output_ch_id(unsigned int sel)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_MUL_CH_CFG_REG_OFF);

	return ((reg_val & CUR_OUT_CH_MASK) >> CUR_OUT_CH);
}


/** get ve online info **/
unsigned int csic_get_ve_frame_st_cnt(unsigned int sel)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_VE_FRM_CNT_REG_OFF);

	return ((reg_val & FRM_ST_CNT_MASK) >> FRM_ST_CNT);
}

unsigned int csic_get_ve_frame_done_cnt(unsigned int sel)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_VE_FRM_CNT_REG_OFF);

	return ((reg_val & FRM_DONE_CNT_MASK) >> FRM_DONE_CNT);
}

unsigned int csic_get_ve_line_st_cnt(unsigned int sel)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_VE_LINE_CNT_REG_OFF);

	return ((reg_val & LINE_ST_CNT_MASK) >> LINE_ST_CNT);
}

unsigned int csic_get_ve_line_done_cnt(unsigned int sel)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_VE_LINE_CNT_REG_OFF);

	return ((reg_val & LINE_DONE_CNT_MASK) >> LINE_DONE_CNT);
}

unsigned int csic_get_ve_cur_frame_addr(unsigned int sel)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_VE_CUR_FRM_ADDR_REG_OF);

	return ((reg_val & CUR_FRM_ADDR_MASK) >> CUR_FRM_ADDR);
}

unsigned int csic_get_ve_last_frame_addr(unsigned int sel)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_VE_LAST_FRM_ADDR_REG_OFF);

	return ((reg_val & LAST_FRM_ADDR_MASK) >> LAST_FRM_ADDR);
}

/* top interrupt en and cfg  */
void csic_dma_top_interrupt_en(unsigned int sel, unsigned int mask)
{
	vin_reg_set(csic_dma_base[sel] + CSIC_DMA_TOP_INT_EN_REG_OFF, mask & DMA_TOP_INT_ALL);
}

void csic_dma_top_interrupt_disable(unsigned int sel, unsigned int mask)
{
	vin_reg_clr(csic_dma_base[sel] + CSIC_DMA_TOP_INT_EN_REG_OFF, mask & DMA_TOP_INT_ALL);
}

void csic_get_dma_top_interrupt_status(unsigned int sel, struct dma_top_int_status *status)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_TOP_INT_STA_REG_OFF);

	status->mask = reg_val;
	status->fsync_sig_rec = (reg_val & FS_PUL_INT_PD_MASK) >> FS_PUL_INT_PD;
	status->clr_fs_cnt = (reg_val & CLR_FS_FRM_CNT_INT_PD_MASK) >> CLR_FS_FRM_CNT_INT_PD;
	status->video_inp_to = (reg_val & VI_INP_TO_INT_PD_MASK) >> VI_INP_TO_INT_PD;
}

void csic_get_dma_version(unsigned int sel, struct dma_version *version)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_VER_REG_OFF);

	version->big_ver = (reg_val & VER_BIG_VER_MASK) >> VER_BIG_VER;
	version->small_ver = (reg_val & VER_SMALL_VER_MASK) >> VER_SMALL_VER;
}


/* Multichannel */
void __csic_buf_addr_fifo_en(unsigned int sel, unsigned int ch, unsigned int en)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_EN_REG_OFF,
			BUF_ADDR_MODE_MASK, en << BUF_ADDR_MODE);
}

void csic_buf_addr_fifo_en(unsigned int sel, unsigned int en)
{
	__csic_buf_addr_fifo_en(sel, dma_virtual_find_ch[sel], en);
}

static void __csic_dma_enable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_EN_REG_OFF,
			CAP_EN_MASK, 1 << CAP_EN);
}

void csic_dma_enable(unsigned int sel)
{
	__csic_dma_enable(sel, dma_virtual_find_ch[sel]);
}

static void __csic_dma_disable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_EN_REG_OFF,
			CAP_EN_MASK, 0 << CAP_EN);
}

void csic_dma_disable(unsigned int sel)
{
	__csic_dma_disable(sel, dma_virtual_find_ch[sel]);
}

static void __csic_lbc_enable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_EN_REG_OFF,
			CAP_EN_MASK, 1 << CAP_EN);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_EN_REG_OFF,
			LBC_EN_MASK, 1 << LBC_EN);
}

void csic_lbc_enable(unsigned int sel)
{
	__csic_lbc_enable(sel, dma_virtual_find_ch[sel]);
}

static void __csic_lbc_disable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_EN_REG_OFF,
			CAP_EN_MASK, 0 << CAP_EN);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_EN_REG_OFF,
			LBC_EN_MASK, 0 << LBC_EN);
}

void csic_lbc_disable(unsigned int sel)
{
	__csic_lbc_disable(sel, dma_virtual_find_ch[sel]);
}


static void __csic_fbc_enable(unsigned int sel, unsigned int ch)
{
}

void csic_fbc_enable(unsigned int sel)
{
	__csic_fbc_enable(sel, dma_virtual_find_ch[sel]);
}

static void __csic_fbc_disable(unsigned int sel, unsigned int ch)
{
}

void csic_fbc_disable(unsigned int sel)
{
	__csic_fbc_disable(sel, dma_virtual_find_ch[sel]);
}

static void __csic_dma_frame_drop_en(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_EN_REG_OFF,
			FRM_DROP_EN_MASK, 1 << FRM_DROP_EN);
}

void csic_dma_frame_drop_en(unsigned int sel)
{
	__csic_dma_frame_drop_en(sel, dma_virtual_find_ch[sel]);
}

static void __csic_dma_frame_drop_disable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_EN_REG_OFF,
			FRM_DROP_EN_MASK, 0 << FRM_DROP_EN);
}

void csic_dma_frame_drop_disable(unsigned int sel)
{
	__csic_dma_frame_drop_disable(sel, dma_virtual_find_ch[sel]);
}

static void __csic_dma_output_fmt_cfg(unsigned int sel, unsigned int ch, enum output_fmt fmt)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CFG_REG_OFF,
			OUTPUT_FMT_MASK, fmt << OUTPUT_FMT);
}

void csic_dma_output_fmt_cfg(unsigned int sel, enum output_fmt fmt)
{
	__csic_dma_output_fmt_cfg(sel, dma_virtual_find_ch[sel], fmt);
}

static void __csic_dma_flip_en(unsigned int sel, unsigned int ch, struct csic_dma_flip *flip)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CFG_REG_OFF,
			HFLIP_EN_MASK, flip->hflip_en << HFLIP_EN);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CFG_REG_OFF,
			VFLIP_EN_MASK, flip->vflip_en << VFLIP_EN);
}

void csic_dma_flip_en(unsigned int sel, struct csic_dma_flip *flip)
{
	__csic_dma_flip_en(sel, dma_virtual_find_ch[sel], flip);
}

static void __csic_dma_cap_mask_num(unsigned int sel, unsigned int ch, unsigned int num)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CFG_REG_OFF,
			CAP_MASK_NUM_MASK, num << CAP_MASK_NUM);
}

void csic_dma_cap_mask_num(unsigned int sel, unsigned int num)
{
	__csic_dma_cap_mask_num(sel, dma_virtual_find_ch[sel], num);
}


static void __csic_dma_config(unsigned int sel, unsigned int ch, struct csic_dma_cfg *cfg)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CFG_REG_OFF,
			FPS_DS_MASK, cfg->ds << FPS_DS);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CFG_REG_OFF,
			FIELD_SEL_MASK,	cfg->field << FIELD_SEL);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CFG_REG_OFF,
			HFLIP_EN_MASK, cfg->flip.hflip_en << HFLIP_EN);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CFG_REG_OFF,
			VFLIP_EN_MASK, cfg->flip.vflip_en << VFLIP_EN);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CFG_REG_OFF,
			OUTPUT_FMT_MASK, cfg->fmt << OUTPUT_FMT);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CFG_REG_OFF,
			CAP_MASK_NUM_MASK, cfg->mask_num << CAP_MASK_NUM);
}

void csic_dma_config(unsigned int sel, struct csic_dma_cfg *cfg)
{
	__csic_dma_config(sel, dma_virtual_find_ch[sel], cfg);
}

static void __csic_dma_10bit_cut2_8bit_enable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CFG_REG_OFF,
				ENABLE_10BIT_CUT2_8BIT_MASK, 1 << ENABLE_10BIT_CUT2_8BIT);
}

void csic_dma_10bit_cut2_8bit_enable(unsigned int sel)
{
	__csic_dma_10bit_cut2_8bit_enable(sel, dma_virtual_find_ch[sel]);
}

static void __csic_dma_10bit_cut2_8bit_disable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CFG_REG_OFF,
				ENABLE_10BIT_CUT2_8BIT_MASK, 0 << ENABLE_10BIT_CUT2_8BIT);
}

void csic_dma_10bit_cut2_8bit_disable(unsigned int sel)
{
	__csic_dma_10bit_cut2_8bit_disable(sel, dma_virtual_find_ch[sel]);
}

static void __csic_frame_lost_cnt_en(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LOST_FRM_CNT_REG_OFF,
				FRM_LOST_CNT_EN_MASK, 1 << FRM_LOST_CNT_EN);
}

void csic_frame_lost_cnt_en(unsigned int sel)
{
	__csic_frame_lost_cnt_en(sel, dma_virtual_find_ch[sel]);
}

static unsigned int __csic_get_frame_lost_cnt(unsigned int sel, unsigned int ch)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LOST_FRM_CNT_REG_OFF);

	return ((reg_val & FRM_LOST_CNT_MASK) >> FRM_LOST_CNT);
}

unsigned int csic_get_frame_lost_cnt(unsigned int sel)
{
	return __csic_get_frame_lost_cnt(sel, dma_virtual_find_ch[sel]);
}

static void __csic_set_threshold_for_bufa_mode(unsigned int sel, unsigned int ch, struct dma_bufa_threshold *threshold)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_BUF_THRESHOLD_REG_OFF,
				BUF_ADDR_FIFO_THRESHOLD_MASK, threshold->bufa_fifo_threshold << BUF_ADDR_FIFO_THRESHOLD);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_BUF_THRESHOLD_REG_OFF,
				STORED_FRM_THRESHOLD_MASK, threshold->stored_frm_threshold << STORED_FRM_THRESHOLD);
}

void csic_set_threshold_for_bufa_mode(unsigned int sel, struct dma_bufa_threshold *threshold)
{
	__csic_set_threshold_for_bufa_mode(sel, dma_virtual_find_ch[sel], threshold);
}

static void __csic_total_frm_cnt_enable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_TOT_FRM_CNT_CFG_REG_OFF,
				TOTAL_FRM_CNT_EN_MASK, 1 << TOTAL_FRM_CNT_EN);
}

void csic_total_frm_cnt_enable(unsigned int sel)
{
	__csic_total_frm_cnt_enable(sel, dma_virtual_find_ch[sel]);
}

static void __csic_total_frm_cnt_disable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_TOT_FRM_CNT_CFG_REG_OFF,
				TOTAL_FRM_CNT_EN_MASK, 0 << TOTAL_FRM_CNT_EN);
}

void csic_total_frm_cnt_disable(unsigned int sel)
{
	__csic_total_frm_cnt_disable(sel, dma_virtual_find_ch[sel]);
}

static void __csic_total_frm_cnt_clear(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_TOT_FRM_CNT_CFG_REG_OFF,
				TOTAL_FRM_CNT_CLR_MASK, 1 << TOTAL_FRM_CNT_CLR);
}

void csic_total_frm_cnt_clear(unsigned int sel)
{
	__csic_total_frm_cnt_clear(sel, dma_virtual_find_ch[sel]);
}

static void __csic_total_frm_cnt_mode(unsigned int sel, unsigned int ch, enum frmae_cnt_mode mode)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_TOT_FRM_CNT_CFG_REG_OFF,
				TOTAL_FRM_CNT_SEL_MASK, mode << TOTAL_FRM_CNT_SEL);
}

void csic_total_frm_cnt_mode(unsigned int sel, enum frmae_cnt_mode mode)
{
	__csic_total_frm_cnt_mode(sel, dma_virtual_find_ch[sel], mode);
}

static void __csic_total_frm_cnt(unsigned int sel, unsigned int ch, unsigned long *count)
{
	*count = vin_reg_readl(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF +  CSIC_DMA_TOL_FRM_CNT_REG_OFF);
}

void csic_total_frm_cnt(unsigned int sel, unsigned long *count)
{
	__csic_total_frm_cnt(sel, dma_virtual_find_ch[sel], count);
}

static void __csic_dma_output_size_cfg(unsigned int sel, unsigned int ch, struct dma_output_size *size)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_HSIZE_REG_OFF,
			HOR_START_MASK, size->hor_start << HOR_START);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_HSIZE_REG_OFF,
			HOR_LEN_MASK, size->hor_len << HOR_LEN);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_VSIZE_REG_OFF,
			VER_START_MASK, size->ver_start << VER_START);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_VSIZE_REG_OFF,
			VER_LEN_MASK, size->ver_len << VER_LEN);
}

void csic_dma_output_size_cfg(unsigned int sel, struct dma_output_size *size)
{
	__csic_dma_output_size_cfg(sel, dma_virtual_find_ch[sel], size);
}

static void __csic_dma_buffer_address(unsigned int sel, unsigned int ch, enum csi_buf_sel buf,
				unsigned long addr)
{
#ifndef BUF_AUTO_UPDATE
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF +  CSIC_DMA_F0_BUFA_REG_OFF + buf * 4,
			F0_BUFA_MASK, addr >> ADDR_BIT_R_SHIFT);
#else
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF +  CSIC_DMA_BUFA_F0_ENTRY_REG_OFF + buf * 2,
			BUFA_F0_ENTRY_MASK, addr >> ADDR_BIT_R_SHIFT);
#endif
}

void csic_dma_buffer_address(unsigned int sel, enum csi_buf_sel buf, unsigned long addr)
{
	__csic_dma_buffer_address(sel, dma_virtual_find_ch[sel], buf, addr);
}

static void __csic_dma_get_buffer_address(unsigned int sel, unsigned int ch, enum csi_buf_sel buf,
				unsigned int *addr)
{
#ifndef BUF_AUTO_UPDATE
	*addr = vin_reg_readl(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF +  CSIC_DMA_F0_BUFA_REG_OFF + buf * 4);
#else
	*addr = vin_reg_readl(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF +  CSIC_DMA_BUFA_F0_ENTRY_REG_OFF + buf * 2);
#endif
	*addr = *addr << ADDR_BIT_R_SHIFT;
}

void csic_dma_get_buffer_address(unsigned int sel, enum csi_buf_sel buf, unsigned int *addr)
{
	__csic_dma_get_buffer_address(sel, dma_virtual_find_ch[sel], buf, addr);
}

static void __csic_dma_buffer_length(unsigned int sel, unsigned int ch, struct dma_buf_len *buf_len)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_BUF_LEN_REG_OFF,
			BUF_LEN_MASK, buf_len->buf_len_y << BUF_LEN);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_BUF_LEN_REG_OFF,
			BUF_LEN_C_MASK,	buf_len->buf_len_c << BUF_LEN_C);
}

void csic_dma_buffer_length(unsigned int sel, struct dma_buf_len *buf_len)
{
	__csic_dma_buffer_length(sel, dma_virtual_find_ch[sel], buf_len);
}

static void __csic_dma_flip_size(unsigned int sel, unsigned int ch, struct dma_flip_size *flip_size)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_FLIP_SIZE_REG_OFF,
			VALID_LEN_MASK, flip_size->hor_len << VALID_LEN);
	vin_reg_clr_set(csic_dma_base[sel]+CSIC_DMA_FLIP_SIZE_REG_OFF,
			VER_LEN_MASK, flip_size->ver_len << VER_LEN);
}

void csic_dma_flip_size(unsigned int sel, struct dma_flip_size *flip_size)
{
	__csic_dma_flip_size(sel, dma_virtual_find_ch[sel], flip_size);
}

static void __csic_dma_cap_status(unsigned int sel, unsigned int ch, struct dma_capture_status *status)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_CAP_STA_REG_OFF);

	status->scap_sta = (reg_val & SCAP_STA_MASK) >> SCAP_STA;
	status->field_sta = (reg_val & FIELD_STA_MASK) >> FIELD_STA;
}


void csic_dma_cap_status(unsigned int sel, struct dma_capture_status *status)
{
	__csic_dma_cap_status(sel, dma_virtual_find_ch[sel], status);
}


static void __csic_dma_int_enable(unsigned int sel, unsigned int ch, enum dma_int_sel interrupt)
{
#ifndef MULTI_FRM_MERGE_INT
	vin_reg_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_INT_EN_REG_OFF, (interrupt & DMA_INT_ALL));
#endif
}

void csic_dma_int_enable(unsigned int sel, enum dma_int_sel interrupt)
{
	__csic_dma_int_enable(sel, dma_virtual_find_ch[sel], interrupt);
}

static void __csic_dma_int_disable(unsigned int sel, unsigned int ch, enum dma_int_sel interrupt)
{
	vin_reg_clr(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_INT_EN_REG_OFF, (interrupt & DMA_INT_ALL));
}

void csic_dma_int_disable(unsigned int sel, enum dma_int_sel interrupt)
{
	__csic_dma_int_disable(sel, dma_virtual_find_ch[sel], interrupt);
}

static void __csic_dma_int_get_status(unsigned int sel, unsigned int ch,
							struct dma_int_status *status)
{
	unsigned int reg_val;
	if (csic_dma_base[sel] == NULL)
		return;

	reg_val = vin_reg_readl(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_INT_STA_REG_OFF);
	memset(status, 0, sizeof(struct dma_int_status));
	status->mask = reg_val;
	status->capture_done = (reg_val & CD_PD_MASK) >> CD_PD;
	status->frame_done = (reg_val & FD_PD_MASK) >> FD_PD;
	status->buf_0_overflow = (reg_val & FIFO0_OF_PD_MASK) >> FIFO0_OF_PD;
	status->line_cnt_flag = (reg_val & LC_PD_MASK) >> LC_PD;
	status->hblank_overflow = (reg_val & HB_OF_INT_EN_MASK) >> HB_OF_INT_EN;
	status->vsync_trig = (reg_val & VS_INT_EN_MASK) >> VS_INT_EN;
	status->buf_addr_fifo = (reg_val & BUF_ADDR_FIFO_INT_PD_MASK) >> BUF_ADDR_FIFO_INT_PD;
	status->stored_frm_cnt = (reg_val & STORED_FRM_CNT_INT_PD_MASK) >> STORED_FRM_CNT_INT_PD;
	status->frm_lost = (reg_val & FRM_LOST_INT_PD_MASK) >> FRM_LOST_INT_PD;
	status->lbc_hb = (reg_val & LBC_HB_INT_PD_MASK) >> LBC_HB_INT_PD;
	status->addr_no_ready = (reg_val & BUF_ADDR_UNDERFLOW_INT_PD_MASK) >> BUF_ADDR_UNDERFLOW_INT_PD;
	status->addr_overflow = (reg_val & BUF_ADDR_OVERFLOW_INT_PD_MASK) >> BUF_ADDR_OVERFLOW_INT_PD;
	status->pixel_miss = (reg_val & PIXEL_MISS_INT_PD_MASK) >> PIXEL_MISS_INT_PD;
	status->line_miss = (reg_val & LINE_MISS_INT_PD_MASK) >> LINE_MISS_INT_PD;
}

void csic_dma_int_get_status(unsigned int sel, struct dma_int_status *status)
{
	__csic_dma_int_get_status(sel, dma_virtual_find_ch[sel], status);
}

unsigned int csic_dma_get_pclk_cnt_line_min(unsigned int sel)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_PCLK_STAT_REG_OFF);

	return ((reg_val & PCLK_CNT_LINE_MIN_MASK) >> PCLK_CNT_LINE_MIN);
}

unsigned int csic_dma_get_pclk_cnt_line_max(unsigned int sel)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_PCLK_STAT_REG_OFF);

	return ((reg_val & PCLK_CNT_LINE_MAX_MASK) >> PCLK_CNT_LINE_MAX);
}

void csic_dma_int_get_status_and_chn(unsigned int sel,
		struct dma_int_status *status, unsigned int *id)
{
	struct dma_int_status sta;
	int logic_id = dma_virtual_find_logic[sel];
	unsigned int i;

	for (i = logic_id; i < (logic_id + 4); i++) {
		csic_dma_int_get_status(i, &sta);
		if (sta.mask) {
			*id = i;
			memcpy(status, &sta, sizeof(struct dma_int_status));
			return;
		}
	}
}

static void __csic_dma_line_cnt(unsigned int sel, unsigned int ch, int line)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_LINE_CNT_REG_OFF,
			LINE_CNT_NUM_MASK, line);
}

void csic_dma_line_cnt(unsigned int sel, int line)
{
	__csic_dma_line_cnt(sel, dma_virtual_find_ch[sel], line);
}

static void __csic_dma_frm_cnt(unsigned int sel, unsigned int ch, struct csi_sync_ctrl *sync)
{
}

void csic_dma_frm_cnt(unsigned int sel, struct csi_sync_ctrl *sync)
{
	__csic_dma_frm_cnt(sel, dma_virtual_find_ch[sel], sync);
}

static void __csic_dma_int_clear_status(unsigned int sel, unsigned int ch, enum dma_int_sel interrupt)
{
	vin_reg_writel(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_INT_STA_REG_OFF, interrupt);
	/* make sure status is cleared */
	vin_reg_readl(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_DMA_INT_STA_REG_OFF);
}

void csic_dma_int_clear_status(unsigned int sel, enum dma_int_sel interrupt)
{
	__csic_dma_int_clear_status(sel, dma_virtual_find_ch[sel], interrupt);
}

void csic_dma_int_clear_all_status(unsigned int sel, enum dma_int_sel interrupt)
{
	unsigned char i;

	for (i = 0; i < DMA_VIRTUAL_NUM; i++)
		__csic_dma_int_clear_status(sel, i, interrupt);
}

static void __csic_lbc_cmp_ratio(unsigned int sel, unsigned int ch, struct dma_lbc_cmp *lbc_cmp)
{
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_CONFIGURE_REG_OFF,
				WHETHER_LOSSY_ENABLE_MASK, lbc_cmp->is_lossy << WHETHER_LOSSY_ENABLE);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_CONFIGURE_REG_OFF,
				GLB_ENABLE_MASK, lbc_cmp->glb_enable << GLB_ENABLE);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_CONFIGURE_REG_OFF,
				DTS_ENABLE_MASK, lbc_cmp->dts_enable << DTS_ENABLE);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_CONFIGURE_REG_OFF,
				OTS_ENABLE_MASK, lbc_cmp->ots_enable << OTS_ENABLE);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_CONFIGURE_REG_OFF,
				MSQ_ENABLE_MASK, lbc_cmp->msq_enable << MSQ_ENABLE);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_CONFIGURE_REG_OFF,
				UPDATE_ADVANTURE_ENABLE_MASK, lbc_cmp->updata_adv_en << UPDATE_ADVANTURE_ENABLE);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_CONFIGURE_REG_OFF,
				UPDATE_ADVANTURE_RATIO_MASK, lbc_cmp->updata_adv_ratio << UPDATE_ADVANTURE_RATIO);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_CONFIGURE_REG_OFF,
				LIMIT_QP_ENABLE_MASK, lbc_cmp->lmtqp_en << LIMIT_QP_ENABLE);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_CONFIGURE_REG_OFF,
				LIMIT_QP_MIM_MASK, lbc_cmp->lmtqp_min << LIMIT_QP_MIM);

	vin_reg_writel(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_LINE_TARGET_BIT0_REG_OFF, lbc_cmp->line_tar_bits[0]);
	vin_reg_writel(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_LINE_TARGET_BIT1_REG_OFF, lbc_cmp->line_tar_bits[1]);

	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_RC_ADV_REG_OFF,
				RATE_CONTROL_ADVANTURE_0_MASK, lbc_cmp->rc_adv[0] << RATE_CONTROL_ADVANTURE_0);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_RC_ADV_REG_OFF,
				RATE_CONTROL_ADVANTURE_1_MASK, lbc_cmp->rc_adv[1] << RATE_CONTROL_ADVANTURE_1);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_RC_ADV_REG_OFF,
				RATE_CONTROL_ADVANTURE_2_MASK, lbc_cmp->rc_adv[2] << RATE_CONTROL_ADVANTURE_2);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_RC_ADV_REG_OFF,
				RATE_CONTROL_ADVANTURE_3_MASK, lbc_cmp->rc_adv[3] << RATE_CONTROL_ADVANTURE_3);

	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_MB_MIN_REG_OFF,
				MACROBLOCK_MIN_BITS0_MASK, lbc_cmp->mb_mi_bits[0] << MACROBLOCK_MIN_BITS0);
	vin_reg_clr_set(csic_dma_base[sel] + ch * CSIC_DMA_CH_OFF + CSIC_LBC_MB_MIN_REG_OFF,
				MACROBLOCK_MIN_BITS1_MASK, lbc_cmp->mb_mi_bits[1] << MACROBLOCK_MIN_BITS1);
}

void csic_lbc_cmp_ratio(unsigned int sel, struct dma_lbc_cmp *lbc_cmp)
{
	__csic_lbc_cmp_ratio(sel, dma_virtual_find_ch[sel], lbc_cmp);
}

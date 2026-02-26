/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 *
 * Authors:  Zheng Zequn <zequnzheng@allwinnertech.com>
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
#include "tdm200_reg.h"

#include "../../utility/vin_io.h"
#include "../../platform/platform_cfg.h"

volatile void __iomem *csic_tdm_base[VIN_MAX_TDM];

#define TDM_ADDR_BIT_R_SHIFT 2
int csic_tdm_set_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel > VIN_MAX_TDM - 1)
		return -1;
	csic_tdm_base[sel] = (volatile void __iomem *)addr;

	return 0;
}

/*
 * function about tdm top registers
 */
void csic_tdm_top_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			TDM_TOP_EN_MASK, 1 << TDM_TOP_EN);
}

void csic_tdm_top_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			TDM_TOP_EN_MASK, 0 << TDM_TOP_EN);
}

void csic_tdm_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			TDM_EN_MASK, 1 << TDM_EN);
}

void csic_tdm_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			TDM_EN_MASK, 0 << TDM_EN);
}


void csic_tdm_vgm_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			VGM_EN_MASK, 1 << VGM_EN);
}

void csic_tdm_vgm_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			VGM_EN_MASK, 0 << VGM_EN);
}

void csic_tdm_set_speed_dn(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			TDM_SPEED_DN_EN_MASK, en << TDM_SPEED_DN_EN);
}

void csic_tdm_fifo_max_layer_en(unsigned int sel, unsigned int en)
{

}

void csic_tdm_set_rx_chn_cfg_mode(unsigned int sel, enum  rx_chn_cfg_mode mode)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			RX_CHN_CFG_MODE_MASK, mode << RX_CHN_CFG_MODE);
}

void csic_tdm_set_tx_chn_cfg_mode(unsigned int sel, enum  tx_chn_cfg_mode mode)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			TX_CHN_CFG_MODE_MASK, mode << TX_CHN_CFG_MODE);
}

void csic_tdm_set_work_mode(unsigned int sel, enum tdm_work_mode mode)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			RX_WORK_MODE_MASK, mode << RX_WORK_MODE);
}

void csic_tdm_set_mod_clk_back_door(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			MODULE_CLK_BACK_DOOR_MASK, en << MODULE_CLK_BACK_DOOR);
}

void csic_tdm_set_line_fresh(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			RX_DATA_LINE_FRESH_EN_MASK, en << RX_DATA_LINE_FRESH_EN);
	vin_reg_clr_set(csic_tdm_base[sel] + TDM_GOLBAL_CFG0_REG_OFF,
			TX_DATA_LINE_FRESH_EN_MASK, en << TX_DATA_LINE_FRESH_EN);
}

void csic_tdm_int_enable(unsigned int sel,	enum tdm_int_sel interrupt)
{
	vin_reg_set(csic_tdm_base[sel] + TDM_INT_BYPASS0_REG_OFF, interrupt);
}

void csic_tdm_int_disable(unsigned int sel, enum tdm_int_sel interrupt)
{
	vin_reg_clr(csic_tdm_base[sel] + TDM_INT_BYPASS0_REG_OFF, interrupt);
}

void csic_tdm_int_get_status(unsigned int sel, struct tdm_int_status *status)
{
	unsigned int reg_val = vin_reg_readl(csic_tdm_base[sel] + TDM_INT_STATUS0_REG_OFF);

	status->rx_frm_lost = (reg_val & RX_FRM_LOST_PD_MASK) >> RX_FRM_LOST_PD;
	status->rx_frm_err = (reg_val & RX_FRM_ERR_PD_MASK) >> RX_FRM_ERR_PD;
	status->rx_btype_err = (reg_val & RX_BTYPE_ERR_PD_MASK) >> RX_BTYPE_ERR_PD;
	status->rx_buf_full = (reg_val & RX_BUF_FULL_PD_MASK) >> RX_BUF_FULL_PD;
	status->rx_hb_short = (reg_val & RX_HB_SHORT_PD_MASK) >> RX_HB_SHORT_PD;
	status->rx_fifo_full = (reg_val & RX_FIFO_FULL_PD_MASK) >> RX_FIFO_FULL_PD;
	status->tdm_lbc_err = (reg_val & TDM_LBC_ERROR_PD_MASK) >> TDM_LBC_ERROR_PD;
	status->tx_fifo_under = (reg_val & TX_FIFO_UNDER_PD_MASK) >> TX_FIFO_UNDER_PD;
	status->rx0_frm_done = (reg_val & RX0_FRM_DONE_PD_MASK) >> RX0_FRM_DONE_PD;
	status->rx1_frm_done = (reg_val & RX1_FRM_DONE_PD_MASK) >> RX1_FRM_DONE_PD;
	status->rx2_frm_done = (reg_val & RX2_FRM_DONE_PD_MASK) >> RX2_FRM_DONE_PD;
	status->rx3_frm_done = (reg_val & RX3_FRM_DONE_PD_MASK) >> RX3_FRM_DONE_PD;
	status->tx_frm_done = (reg_val & TX_FRM_DONE_PD_MASK) >> TX_FRM_DONE_PD;
	status->speed_dn_fifo_full = (reg_val & SPEED_DN_FIFO_FULL_PD_MASK) >> SPEED_DN_FIFO_FULL_PD;
	status->speed_dn_hsync = (reg_val & SPEED_DN_HSYN_PD_MASK) >> SPEED_DN_HSYN_PD;
	status->rx_chn_cfg_mode = (reg_val & RX_CHN_CFG_MODE_PD_MASK) >> RX_CHN_CFG_MODE_PD;
	status->tx_chn_cfg_mode = (reg_val & TX_CHN_CFG_MODE_PD_MASK) >> TX_CHN_CFG_MODE_PD;
	status->tdm_lbc_fifo_full = (reg_val & TDM_LBC_FIFO_FULL_PD_MASK) >> TDM_LBC_FIFO_FULL_PD;
}

void csic_tdm_int_clear_status(unsigned int sel, enum tdm_int_sel interrupt)
{
	vin_reg_writel(csic_tdm_base[sel] + TDM_INT_STATUS0_REG_OFF, interrupt);
}

unsigned int csic_tdm_internal_get_status(unsigned int sel, unsigned int status)
{
	return (vin_reg_readl(csic_tdm_base[sel] + TDM_INT_STATUS0_REG_OFF)) & status;
}
unsigned int csic_tdm_internal_get_status0(unsigned int sel, unsigned int status)
{
	return (vin_reg_readl(csic_tdm_base[sel] + TDM_INTERNAL_STATUS0_REG_OFF)) & status;
}

void csic_tdm_internal_clear_status0(unsigned int sel, unsigned int status)
{
	vin_reg_writel(csic_tdm_base[sel] + TDM_INTERNAL_STATUS0_REG_OFF, status);
}

unsigned int csic_tdm_internal_get_status1(unsigned int sel, unsigned int status)
{
	return (vin_reg_readl(csic_tdm_base[sel] + TDM_INTERNAL_STATUS1_REG_OFF)) & status;
}

void csic_tdm_internal_clear_status1(unsigned int sel, unsigned int status)
{
	vin_reg_writel(csic_tdm_base[sel] + TDM_INTERNAL_STATUS1_REG_OFF, status);
}

unsigned int csic_tdm_internal_get_status2(unsigned int sel, unsigned int status)
{
	return (vin_reg_readl(csic_tdm_base[sel] + TDM_INTERNAL_STATUS2_REG_OFF)) & status;
}

void csic_tdm_internal_clear_status2(unsigned int sel, unsigned int status)
{
	vin_reg_writel(csic_tdm_base[sel] + TDM_INTERNAL_STATUS2_REG_OFF, status);
}

/*
 * function about tdm tx registers
 */
#if VIN_FALSE
void csic_tdm_set_vgm_data_mode(unsigned int sel, enum vgm_data_mode mode)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_VGM_OFFSET + TDM_VGM_CFG0_REG_OFF,
			VGM_DMODE_MASK, mode << VGM_DMODE);
}

void csic_tdm_set_vgm_smode(unsigned int sel, enum vgm_smode mode)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_VGM_OFFSET + TDM_VGM_CFG0_REG_OFF,
			VGM_SMODE_MASK, mode << VGM_SMODE);
}

void csic_tdm_vgm_start(unsigned int sel)
{
	 vin_reg_clr_set(csic_tdm_base[sel] + TMD_VGM_OFFSET + TDM_VGM_CFG0_REG_OFF,
			 VGM_START_MASK, 1 << VGM_START);
}

void csic_tdm_vgm_stop(unsigned int sel, enum vgm_data_mode mode)
{
	 vin_reg_clr_set(csic_tdm_base[sel] + TMD_VGM_OFFSET + TDM_VGM_CFG0_REG_OFF,
			 VGM_START_MASK, 0 << VGM_START);
}

void csic_tdm_set_vgm_bcycle(unsigned int sel, unsigned int cycle)
{
	 vin_reg_clr_set(csic_tdm_base[sel] + TMD_VGM_OFFSET + TDM_VGM_CFG0_REG_OFF,
			 VGM_BCYCLE_MASK, cycle << VGM_BCYCLE);
}

void csic_tdm_set_vgm_input_fmt(unsigned int sel, enum tdm_input_fmt fmt)
{
	 vin_reg_clr_set(csic_tdm_base[sel] + TMD_VGM_OFFSET + TDM_VGM_CFG1_REG_OFF,
			 VGM_INPUT_FMT_MASK, fmt << VGM_INPUT_FMT);
}

void csic_tdm_set_vgm_data_type(unsigned int sel, enum vgm_data_type type)
{
	 vin_reg_clr_set(csic_tdm_base[sel] + TMD_VGM_OFFSET + TDM_VGM_CFG1_REG_OFF,
			 VGM_DATA_TYPE_MASK, type << VGM_DATA_TYPE);
}

void csic_tdm_set_vgm_para0(unsigned int sel, unsigned int para0, unsigned int para1)
{
	 vin_reg_clr_set(csic_tdm_base[sel] + TMD_VGM_OFFSET + TDM_VGM_CFG2_REG_OFF,
			 VGM_PARA0_MASK, para0 << VGM_PARA0);
	 vin_reg_clr_set(csic_tdm_base[sel] + TMD_VGM_OFFSET + TDM_VGM_CFG2_REG_OFF,
			 VGM_PARA1_MASK, para1 << VGM_PARA1);
}
#endif
/*
 * function about tdm tx registers
 */

void csic_tdm_tx_cap_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_CFG0_REG_OFF,
				TDM_TX_EN_MASK, 1 << TDM_TX_EN);
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_CFG0_REG_OFF,
				TDM_TX_CAP_EN_MASK, 1 << TDM_TX_CAP_EN);
}

void csic_tdm_tx_cap_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_CFG0_REG_OFF,
				TDM_TX_CAP_EN_MASK, 0 << TDM_TX_CAP_EN);
}

void csic_tdm_tx_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_CFG0_REG_OFF,
				TDM_TX_EN_MASK, 1 << TDM_TX_EN);
}

void csic_tdm_tx_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_CFG0_REG_OFF,
				TDM_TX_EN_MASK, 0 << TDM_TX_EN);
}

void csic_tdm_omode(unsigned int sel, unsigned int mode)
{
}

void csic_tdm_fifo_mode(unsigned int sel, unsigned int mode)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_CFG0_REG_OFF,
				TDM_TX_FIFO_MODE_MASK, mode << TDM_TX_FIFO_MODE);
}

void csic_tdm_set_hblank(unsigned int sel, unsigned int hblank)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_CFG1_REG_OFF,
				TDM_TX_H_BLANK_MASK, hblank << TDM_TX_H_BLANK);
}

void csic_tdm_set_bblank_fe(unsigned int sel, unsigned int bblank_fe)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_CFG2_REG_OFF,
				TDM_TX_V_BLANK_FE_MASK, bblank_fe << TDM_TX_V_BLANK_FE);
}

void csic_tdm_set_bblank_be(unsigned int sel, unsigned int bblank_be)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_CFG2_REG_OFF,
				TDM_TX_V_BLANK_BE_MASK, bblank_be << TDM_TX_V_BLANK_BE);
}

void csic_tdm_set_tx_t1_cycle(unsigned int sel, unsigned int cycle)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_TIME1_CYCLE_OFF,
				TDM_TX_T1_CYCLE_MASK, cycle << TDM_TX_T1_CYCLE);
}

void csic_tdm_set_tx_t2_cycle(unsigned int sel, unsigned int cycle)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_TIME2_CYCLE_OFF,
				TDM_TX_T2_CYCLE_MASK, cycle << TDM_TX_T2_CYCLE);
}

void csic_tdm_set_tx_fifo_depth(unsigned int sel, unsigned int head_depth, unsigned int data_depth)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_FIFO_DEPTH_OFF,
				TDM_TX_HEAD_FIFO_MASK, head_depth << TDM_TX_HEAD_FIFO);
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_FIFO_DEPTH_OFF,
				TDM_TX_DATA_FIFO_MASK, data_depth << TDM_TX_DATA_FIFO);
}

#if VIN_FALSE
void csic_tdm_set_tx_invalid_num(unsigned int sel, unsigned int num)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_DATA_RATE_REG_OFF,
				TDM_TX_INVALID_NUM_MASK, num << TDM_TX_INVALID_NUM);
}

void csic_tdm_set_tx_valid_num(unsigned int sel, unsigned int num)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_DATA_RATE_REG_OFF,
				TDM_TX_VALID_NUM_MASK, num << TDM_TX_VALID_NUM);
}
#else
void csic_tdm_set_tx_data_rate(unsigned int sel, unsigned int valid_num, unsigned int invalid_num)
{
	unsigned int num;

	num = ((valid_num & 0xff) << TDM_TX_VALID_NUM) + ((invalid_num & 0xff) << TDM_TX_INVALID_NUM);
	vin_reg_writel(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_DATA_RATE_REG_OFF, num);
}
#endif

unsigned int csic_tdm_get_tx_ctrl_status(unsigned int sel)
{
	return vin_reg_readl(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_CTRL_ST_REG_OFF);
}

void csic_tdm_set_tx_id0_fifo_depth(unsigned int sel, unsigned int head_depth, unsigned int data_depth)
{
	int i = 0;
	for (i = 0; i < 4; i++) {
		vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_ID0_FIFO_DEPTH_OFF + 4 * i,
					TDM_TX_HEAD_FIFO_MASK, head_depth << TDM_TX_HEAD_FIFO);
		vin_reg_clr_set(csic_tdm_base[sel] + TMD_TX_OFFSET + TDM_TX_ID0_FIFO_DEPTH_OFF,
					TDM_TX_DATA_FIFO_MASK, data_depth << TDM_TX_DATA_FIFO);
	}
}

/*
 * function about tdm rx registers
 */
void csic_tdm_rx_enable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_EN_MASK, 1 << TDM_RX_EN);
}

void csic_tdm_rx_disable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_EN_MASK, 0 << TDM_RX_EN);
}

void csic_tdm_rx_cap_enable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_CAP_EN_MASK, 1 << TDM_RX_CAP_EN);
}

void csic_tdm_rx_cap_disable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_CAP_EN_MASK, 0 << TDM_RX_CAP_EN);
}

void csic_tdm_rx_tx_enable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_TX_EN_MASK, 1 << TDM_RX_TX_EN);
}

void csic_tdm_rx_tx_disable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_TX_EN_MASK, 0 << TDM_RX_TX_EN);
}

void csic_tdm_rx_lbc_enable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_LBC_EN_MASK, 1 << TDM_RX_LBC_EN);
}

void csic_tdm_rx_lbc_disable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_LBC_EN_MASK, 0 << TDM_RX_LBC_EN);
}

void csic_tdm_rx_pkg_enable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_PKG_EN_MASK, 1 << TDM_RX_PKG_EN);
}

void csic_tdm_rx_pkg_disable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_PKG_EN_MASK, 0 << TDM_RX_PKG_EN);
}

void csic_tdm_rx_sync_enable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_SYN_EN_MASK, 1 << TDM_RX_SYN_EN);
}
void csic_tdm_rx_sync_disable(unsigned int sel, unsigned int ch)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_SYN_EN_MASK, 0 << TDM_RX_SYN_EN);
}

void csic_tdm_rx_set_line_num_ddr(unsigned int sel, unsigned int ch, unsigned int en)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_LINE_NUM_DDR_EN_MASK, en << TDM_LINE_NUM_DDR_EN);
}

void csic_tdm_rx_set_buf_num(unsigned int sel, unsigned int ch, unsigned int num)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_BUF_NUM_MASK, num << TDM_RX_BUF_NUM);
}

void csic_tdm_rx_ch0_en(unsigned int sel, unsigned int ch, unsigned int en)
{
}

void csic_tdm_rx_set_min_ddr_size(unsigned int sel, unsigned int ch, enum min_ddr_size_sel ddr_size)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_MIN_DDR_SIZE_MASK, ddr_size << TDM_RX_MIN_DDR_SIZE);
}

void csic_tdm_rx_input_fmt(unsigned int sel, unsigned int ch, enum tdm_input_fmt fmt)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_RX_INPUT_FMT_MASK, fmt << TDM_RX_INPUT_FMT);
}

void csic_tdm_rx_input_bit(unsigned int sel, unsigned int ch, enum input_image_type_sel input_tpye)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG0_REG_OFF,
					TDM_INPUT_BIT_MASK, input_tpye << TDM_INPUT_BIT);
}

void csic_tdm_rx_input_size(unsigned int sel, unsigned int ch, unsigned int width, unsigned int height)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG1_REG_OFF,
					TDM_RX_WIDTH_MASK, width << TDM_RX_WIDTH);
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG1_REG_OFF,
					TDM_RX_HEIGHT_MASK, height << TDM_RX_HEIGHT);
}

void csic_tdm_rx_data_fifo_depth(unsigned int sel, unsigned int ch, unsigned int depth)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_FIFO_DEPTH_REG_OFF,
				TDM_RX_DATA_FIFO_DEPTH_MASK, depth << TDM_RX_DATA_FIFO_DEPTH);
}

void csic_tdm_rx_head_fifo_depth(unsigned int sel, unsigned int ch, unsigned int depth)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_FIFO_DEPTH_REG_OFF,
			TDM_RX_HEAD_FIFO_DEPTH_MASK, depth << TDM_RX_HEAD_FIFO_DEPTH);
}

void csic_tdm_rx_data_fifo_clear(unsigned int sel)
{
	int i = 0;
	for (i = 0; i < 4; i++) {
		vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + i*AMONG_RX_OFFSET + TDM_RX_FIFO_DEPTH_REG_OFF,
			TDM_RX_HEAD_FIFO_DEPTH_MASK, 0 << TDM_RX_HEAD_FIFO_DEPTH);
		vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + i*AMONG_RX_OFFSET + TDM_RX_FIFO_DEPTH_REG_OFF,
				TDM_RX_DATA_FIFO_DEPTH_MASK, 0 << TDM_RX_DATA_FIFO_DEPTH);
	}
}

void csic_tdm_rx_pkg_line_words(unsigned int sel, unsigned int ch, unsigned int words)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_FIFO_DEPTH_REG_OFF,
					TDM_RX_PKG_LINE_WORDS_MASK, words << TDM_RX_PKG_LINE_WORDS);
}

void csic_tdm_rx_set_address(unsigned int sel, unsigned int ch, unsigned long address)
{
	vin_reg_writel(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_CFG2_REG_OFF,
					address >> TDM_ADDR_BIT_R_SHIFT);
}

void csic_tdm_rx_get_size(unsigned int sel, unsigned int ch, unsigned int *width, unsigned int *heigth)
{
	unsigned int regval;

	regval = vin_reg_readl(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_FRAME_ERR_REG_OFF);
	*width = regval & 0x3fff;
	*heigth = (regval >> 16) & 0x3fff;
}

void csic_tdm_rx_get_hblank(unsigned int sel, unsigned int ch, unsigned int *hb_min, unsigned int *hb_max)
{
	unsigned int regval;

	regval = vin_reg_readl(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_HB_SHORT_REG_OFF);
	*hb_max = regval & 0xffff;
	*hb_min = (regval >> 16) & 0xffff;
}

void csic_tdm_rx_get_layer(unsigned int sel, unsigned int ch, unsigned int *head_fifo, unsigned int *fifo)
{
	unsigned int regval;

	regval = vin_reg_readl(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_LAYER_REG_OFF);
	*head_fifo = regval & 0x7ff;
	*fifo = (regval >> 16) & 0xffff;
}

unsigned int csic_tdm_rx_get_proc_num(unsigned int sel, unsigned int ch)
{

	return vin_reg_readl(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_PROC_NUM_REG_OFF) & 0x3f;
}

unsigned int csic_tdm_rx_get_ctrl_st(unsigned int sel, unsigned int ch)
{

	return vin_reg_readl(csic_tdm_base[sel] + TMD_RX0_OFFSET + ch*AMONG_RX_OFFSET + TDM_RX_STATUS_REG_OFF) & 0xffff;
}

/*
 * function about tdm lbc registers
 */
void csic_tdm_lbc_cfg(unsigned int sel, unsigned int ch, struct tdm_rx_lbc *lbc)
{
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_LBC0_OFFSET + ch*AMONG_LBC_OFFSET + TMD_LBC_CFG0_REG_OFF,
					IS_LOSSY_MASK, lbc->is_lossy << IS_LOSSY);
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_LBC0_OFFSET + ch*AMONG_LBC_OFFSET + TMD_LBC_CFG0_REG_OFF,
					STATUS_QP_MASK, lbc->start_qp << STATUS_QP);
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_LBC0_OFFSET + ch*AMONG_LBC_OFFSET + TMD_LBC_CFG0_REG_OFF,
					MB_TH_MASK, lbc->mb_th << MB_TH);
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_LBC0_OFFSET + ch*AMONG_LBC_OFFSET + TMD_LBC_CFG0_REG_OFF,
					MB_NUM_MASK, lbc->mb_num << MB_NUM);

	vin_reg_clr_set(csic_tdm_base[sel] + TMD_LBC0_OFFSET + ch*AMONG_LBC_OFFSET + TMD_LBC_CFG1_REG_OFF,
					MB_MIN_BIT_MASK, lbc->mb_min_bit << MB_MIN_BIT);
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_LBC0_OFFSET + ch*AMONG_LBC_OFFSET + TMD_LBC_CFG1_REG_OFF,
					GBGR_RATIO_MASK, lbc->gbgr_ratio << GBGR_RATIO);

	vin_reg_clr_set(csic_tdm_base[sel] + TMD_LBC0_OFFSET + ch*AMONG_LBC_OFFSET + TMD_LBC_CFG2_REG_OFF,
					LINE_TAR_BIT_MASK, lbc->line_tar_bits << LINE_TAR_BIT);
	vin_reg_clr_set(csic_tdm_base[sel] + TMD_LBC0_OFFSET + ch*AMONG_LBC_OFFSET + TMD_LBC_CFG2_REG_OFF,
					LINE_MAX_BIT_MASK, lbc->line_max_bit << LINE_MAX_BIT);


}

void csic_tdm_get_lbc_st(unsigned int sel, unsigned int ch, struct tdm_lbc_status *status)
{
	unsigned int val;

	val = vin_reg_readl(csic_tdm_base[sel] + TMD_LBC0_OFFSET + ch*AMONG_LBC_OFFSET + TMD_LBC_STATUS_REG_OFF);
	status->lbc_over = (val & LBC_OVER_MASK) >> LBC_OVER;
	status->lbc_over = (val & LBC_SHORT_MASK) >> LBC_SHORT;
	status->lbc_over = (val & LBC_QP_MASK) >> LBC_QP;
}

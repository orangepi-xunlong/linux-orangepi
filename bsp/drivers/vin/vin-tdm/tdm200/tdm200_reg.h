/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2019Allwinnertech Co., Ltd.
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

#ifndef __CSIC__TDM200__REG__H__
#define __CSIC__TDM200__REG__H__

#include <media/sunxi_camera_v2.h>
#include <linux/types.h>
#include <media/v4l2-mediabus.h>
#include "tdm200_reg_i.h"
#include "../../platform/platform_cfg.h"

#define TDM_RX_NUM   4

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
#define MAX_TX_HEAD_FIFO_DEPTH		64
#define MAX_TX_DATA_FIFO_DEPTH		320
#define MAX_RX_HEAD_FIFO_DEPTH		64
#define MAX_RX_DATA_FIFO_DEPTH		880
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW6)
#define MAX_TX_HEAD_FIFO_DEPTH		64
#define MAX_TX_DATA_FIFO_DEPTH		160
#define MAX_RX_HEAD_FIFO_DEPTH		64
#define MAX_RX_DATA_FIFO_DEPTH		448
#else /* CONFIG_ARCH_SUN55IW3 && CONFIG_ARCH_SUN60IW1 && CONFIG_ARCH_SUN8IW21 */
#define MAX_TX_HEAD_FIFO_DEPTH		32
#define MAX_TX_DATA_FIFO_DEPTH		512
#define MAX_RX_HEAD_FIFO_DEPTH		32
#define MAX_RX_DATA_FIFO_DEPTH		512
#endif

enum  tx_chn_cfg_mode {
	TX_NONE = 0x0,
	CH0_1,
	CH2_3,
	CH0_1_AND_CH2_3,
	CH0_1_2,
	CH0_1_2_3,
};

enum  rx_chn_cfg_mode {
	LINEARx4 = 0x0,
	WDR_2F_AND_LINEARx2,
	WDR_2Fx2,
	WDR_3F_AND_LINEAR,
};

enum tdm_int_sel {
	RX_FRM_LOST_INT_EN = 0X1 << 0,
	RX_FRM_ERR_INT_EN = 0X1 << 1,
	RX_BTYPE_ERR_INT_EN = 0X1 << 2,
	RX_BUF_FULL_INT_EN = 0X1 << 3,
	RX_HB_SHORT_INT_EN = 0X1 << 5,
	RX_FIFO_FULL_INT_EN = 0X1 << 6,
	TDM_LBC_ERR_INT_EN = 0X1 << 8,
	TDM_FIFO_UNDER_INT_EN = 0X1 << 9,
	RX0_FRM_DONE_INT_EN = 0X1 << 16,
	RX1_FRM_DONE_INT_EN = 0X1 << 17,
	RX2_FRM_DONE_INT_EN = 0X1 << 18,
	RX3_FRM_DONE_INT_EN = 0X1 << 19,
	TX_FRM_DONE_INT_EN = 0X1 << 20,
	SPEED_DN_FIFO_FULL_INT_EN = 0X1 << 22,
	SPEED_DN_HSYNC_INT_EN = 0X1 << 23,
	RX_CHN_CFG_MODE_INT_EN = 0X1 << 24,
	TX_CHN_CFG_MODE_INT_EN = 0X1 << 25,
	RDM_LBC_FIFO_FULL_INT_EN = 0X1 << 26,
	TDM_INT_ALL = 0X7DF036F,
};

enum vgm_smode {
	TRANSF_1F = 0x0,
	TRANSF_CONTINUITY,
};

enum vgm_data_mode {
	ONLY_R = 0,
	ONLY_G,
	ONLy_B,
	ONLY_RGB,
	HOR_COLORBAR,
	VER_COLORBAR,
	RANDOM_DATA,
};

enum vgm_data_type {
	LINEAR_12BIT = 0,
	WDR_2F_16_bit,
	WDR_3F_20BIT,
};

enum tdm_input_fmt {
	BAYER_BGGR = 0x4,
	BAYER_RGGB,
	BAYER_GBRG,
	BAYER_GRBG,
};

enum min_ddr_size_sel {
	DDRSIZE_256b = 0,
	DDRSIZE_512b,
	DDRSIZE_1024b,
	DDRSIZE_2048b,
};

enum input_image_type_sel {
	INPUTTPYE_8BIT = 0,
	INPUTTPYE_10BIT,
	INPUTTPYE_12BIT,
	INPUTTPYE_14BIT,
	INPUTTPYE_16BIT,
	INPUTTPYE_20BIT = 0x6,
	INPUTTPYE_24BIT,
};

enum tdm_work_mode {
	TDM_ONLINE = 0,
	TDM_OFFLINE = 1,
};

struct tdm_int_status {
	bool rx_frm_lost;
	bool rx_frm_err;
	bool rx_btype_err;
	bool rx_buf_full;
	bool rx_hb_short;
	bool rx_fifo_full;
	bool tdm_lbc_err;
	bool tx_fifo_under;
	bool rx0_frm_done;
	bool rx1_frm_done;
	bool rx2_frm_done;
	bool rx3_frm_done;
	bool tx_frm_done;
	bool speed_dn_fifo_full;
	bool speed_dn_hsync;
	bool rx_chn_cfg_mode;
	bool tx_chn_cfg_mode;
	bool tdm_lbc_fifo_full;
	bool rx_comp_err;
};

struct tdm_tx_cfg {
	bool en;
	bool cap_en;
	unsigned int hb;
	unsigned int vb_be;
	unsigned int vb_fe;
	unsigned int t1_cycle;
	unsigned int t2_cycle;
	unsigned int head_depth;
	unsigned int data_depth;
	unsigned int valid_num;
	unsigned int invalid_num;
	unsigned int fifo_mode;
};

struct tdm_rx_lbc {
	unsigned char is_lossy;
	unsigned char start_qp;
	unsigned char mb_th;
	unsigned char mb_num;
	unsigned int mb_min_bit;
	unsigned int gbgr_ratio;
	unsigned int line_tar_bits;
	unsigned int line_max_bit;

	unsigned short cmp_ratio;
};

struct tdm_lbc_status {
	bool lbc_over;
	bool lbc_short;
	bool lbc_qp;
};

int csic_tdm_set_base_addr(unsigned int sel, unsigned long addr);
void csic_tdm_top_enable(unsigned int sel);
void csic_tdm_top_disable(unsigned int sel);
void csic_tdm_enable(unsigned int sel);
void csic_tdm_disable(unsigned int sel);
void csic_tdm_vgm_enable(unsigned int sel);
void csic_tdm_vgm_disable(unsigned int sel);
void csic_tdm_set_speed_dn(unsigned int sel, unsigned int en);
void csic_tdm_fifo_max_layer_en(unsigned int sel, unsigned int en);
void csic_tdm_set_rx_chn_cfg_mode(unsigned int sel, enum  rx_chn_cfg_mode mode);
void csic_tdm_set_tx_chn_cfg_mode(unsigned int sel, enum  tx_chn_cfg_mode mode);
void csic_tdm_set_work_mode(unsigned int sel, enum tdm_work_mode mode);
void csic_tdm_set_mod_clk_back_door(unsigned int sel, unsigned int en);
void csic_tdm_set_line_fresh(unsigned int sel, unsigned int en);
void csic_tdm_int_enable(unsigned int sel,	enum tdm_int_sel interrupt);
void csic_tdm_int_disable(unsigned int sel, enum tdm_int_sel interrupt);
void csic_tdm_int_get_status(unsigned int sel, struct tdm_int_status *status);
void csic_tdm_int_clear_status(unsigned int sel, enum tdm_int_sel interrupt);
unsigned int csic_tdm_internal_get_status0(unsigned int sel, unsigned int status);
void csic_tdm_internal_clear_status0(unsigned int sel, unsigned int status);
unsigned int csic_tdm_internal_get_status(unsigned int sel, unsigned int status);
unsigned int csic_tdm_internal_get_status1(unsigned int sel, unsigned int status);
void csic_tdm_internal_clear_status1(unsigned int sel, unsigned int status);
unsigned int csic_tdm_internal_get_status2(unsigned int sel, unsigned int status);
void csic_tdm_internal_clear_status2(unsigned int sel, unsigned int status);
#if VIN_FALSE
void csic_tdm_set_vgm_data_mode(unsigned int sel, enum vgm_data_mode mode);
void csic_tdm_set_vgm_smode(unsigned int sel, enum vgm_smode mode);
void csic_tdm_vgm_start(unsigned int sel);
void csic_tdm_vgm_stop(unsigned int sel, enum vgm_data_mode mode);
void csic_tdm_set_vgm_bcycle(unsigned int sel, unsigned int cycle);
void csic_tdm_set_vgm_input_fmt(unsigned int sel, enum tdm_input_fmt fmt);
void csic_tdm_set_vgm_data_type(unsigned int sel, enum vgm_data_type type);
void csic_tdm_set_vgm_para0(unsigned int sel, unsigned int para0, unsigned int para1);
#endif
void csic_tdm_tx_cap_enable(unsigned int sel);
void csic_tdm_tx_cap_disable(unsigned int sel);
void csic_tdm_tx_enable(unsigned int sel);
void csic_tdm_tx_disable(unsigned int sel);
void csic_tdm_omode(unsigned int sel, unsigned int mode);
void csic_tdm_fifo_mode(unsigned int sel, unsigned int mode);
void csic_tdm_set_hblank(unsigned int sel, unsigned int hblank);
void csic_tdm_set_bblank_fe(unsigned int sel, unsigned int bblank_fe);
void csic_tdm_set_bblank_be(unsigned int sel, unsigned int bblank_be);
void csic_tdm_set_tx_t1_cycle(unsigned int sel, unsigned int cycle);
void csic_tdm_set_tx_t2_cycle(unsigned int sel, unsigned int cycle);
void csic_tdm_set_tx_fifo_depth(unsigned int sel, unsigned int head_depth, unsigned int data_depth);
#if VIN_FALSE
void csic_tdm_set_tx_invalid_num(unsigned int sel, unsigned int num);
void csic_tdm_set_tx_valid_num(unsigned int sel, unsigned int num);
#else
void csic_tdm_set_tx_data_rate(unsigned int sel, unsigned int valid_num, unsigned int invalid_num);
#endif
unsigned int csic_tdm_get_tx_ctrl_status(unsigned int sel);
void csic_tdm_set_tx_id0_fifo_depth(unsigned int sel, unsigned int head_depth, unsigned int data_depth);
void csic_tdm_rx_enable(unsigned int sel, unsigned int ch);
void csic_tdm_rx_disable(unsigned int sel, unsigned int ch);
void csic_tdm_rx_cap_enable(unsigned int sel, unsigned int ch);
void csic_tdm_rx_cap_disable(unsigned int sel, unsigned int ch);
void csic_tdm_rx_tx_enable(unsigned int sel, unsigned int ch);
void csic_tdm_rx_tx_disable(unsigned int sel, unsigned int ch);
void csic_tdm_rx_lbc_enable(unsigned int sel, unsigned int ch);
void csic_tdm_rx_lbc_disable(unsigned int sel, unsigned int ch);
void csic_tdm_rx_pkg_enable(unsigned int sel, unsigned int ch);
void csic_tdm_rx_pkg_disable(unsigned int sel, unsigned int ch);
void csic_tdm_rx_sync_enable(unsigned int sel, unsigned int ch);
void csic_tdm_rx_sync_disable(unsigned int sel, unsigned int ch);
void csic_tdm_rx_set_line_num_ddr(unsigned int sel, unsigned int ch, unsigned int en);
void csic_tdm_rx_set_buf_num(unsigned int sel, unsigned int ch, unsigned int num);
void csic_tdm_rx_ch0_en(unsigned int sel, unsigned int ch, unsigned int en);
void csic_tdm_rx_set_min_ddr_size(unsigned int sel, unsigned int ch, enum min_ddr_size_sel ddr_size);
void csic_tdm_rx_input_fmt(unsigned int sel, unsigned int ch, enum tdm_input_fmt fmt);
void csic_tdm_rx_input_bit(unsigned int sel, unsigned int ch, enum input_image_type_sel input_tpye);
void csic_tdm_rx_input_size(unsigned int sel, unsigned int ch, unsigned int width, unsigned int height);
void csic_tdm_rx_data_fifo_depth(unsigned int sel, unsigned int ch, unsigned int depth);
void csic_tdm_rx_head_fifo_depth(unsigned int sel, unsigned int ch, unsigned int depth);
void csic_tdm_rx_data_fifo_clear(unsigned int sel);
void csic_tdm_rx_pkg_line_words(unsigned int sel, unsigned int ch, unsigned int words);
void csic_tdm_rx_set_address(unsigned int sel, unsigned int ch, unsigned long address);
void csic_tdm_rx_get_size(unsigned int sel, unsigned int ch, unsigned int *width, unsigned int *heigth);
void csic_tdm_rx_get_hblank(unsigned int sel, unsigned int ch, unsigned int *hb_min, unsigned int *hb_max);
void csic_tdm_rx_get_layer(unsigned int sel, unsigned int ch, unsigned int *head_fifo, unsigned int *fifo);
unsigned int csic_tdm_rx_get_proc_num(unsigned int sel, unsigned int ch);
unsigned int csic_tdm_rx_get_ctrl_st(unsigned int sel, unsigned int ch);
void csic_tdm_lbc_cfg(unsigned int sel, unsigned int ch, struct tdm_rx_lbc *lbc);
void csic_tdm_get_lbc_st(unsigned int sel, unsigned int ch, struct tdm_lbc_status *status);
#endif /* __CSIC__TDM200__REG__H__ */
